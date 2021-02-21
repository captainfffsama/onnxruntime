// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ort_tasks.h"

namespace onnxruntime {
namespace contrib {

void OrtTasks::CreateBackgroundTask(int64_t run_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  ORT_ENFORCE(bg_tasks_.find(run_id) == bg_tasks_.end());
  bg_tasks_[run_id] = std::make_unique<Task>();
}

void OrtTasks::RemoveTask(int64_t run_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  ORT_ENFORCE(bg_tasks_.find(run_id) != bg_tasks_.end());
  bg_tasks_.erase(run_id);
}

void OrtTasks::SetForwardOutputs(Status s, const std::vector<OrtValue>& forward_outputs) {
  int64_t run_id = hasher_(std::this_thread::get_id());
  ORT_ENFORCE(bg_tasks_.find(run_id) != bg_tasks_.end());
  bg_tasks_[run_id]->forward_output_promise_.set_value(std::make_pair(s, forward_outputs));
}

ForwardReturnType OrtTasks::WaitForForwardOutputs(int64_t run_id) {
  ORT_ENFORCE(bg_tasks_.find(run_id) != bg_tasks_.end());
  return bg_tasks_[run_id]->forward_output_future_.get();
}

bool OrtTasks::ForwardOutputsIsValid() {
  int64_t run_id = hasher_(std::this_thread::get_id());
  ORT_ENFORCE(bg_tasks_.find(run_id) != bg_tasks_.end());
  return bg_tasks_[run_id]->forward_output_future_.valid();
}

void OrtTasks::SetBackwardInputs(int64_t run_id, const std::vector<OrtValue>& backward_inputs, bool terminate) {
  ORT_ENFORCE(bg_tasks_.find(run_id) != bg_tasks_.end());
  bg_tasks_[run_id]->backward_input_promise_.set_value(std::make_pair(terminate, backward_inputs));
}

BackwardReturnType OrtTasks::WaitForBackwardInputs() {
  int64_t run_id = hasher_(std::this_thread::get_id());
  ORT_ENFORCE(bg_tasks_.find(run_id) != bg_tasks_.end());
  return bg_tasks_[run_id]->backward_input_future_.get();
}

void OrtTasks::SetStatus(const Status& status) {
  int64_t run_id = hasher_(std::this_thread::get_id());
  ORT_ENFORCE(bg_tasks_.find(run_id) != bg_tasks_.end());
  bg_tasks_[run_id]->status_promise_.set_value(status);
}

bool OrtTasks::TaskIsCompleted(int64_t run_id) {
  ORT_ENFORCE(bg_tasks_.find(run_id) != bg_tasks_.end());
  // if status_future has been invalidated, the task is completed
  return !bg_tasks_[run_id]->status_future_.valid();
}

Status OrtTasks::WaitForStatus(int64_t run_id) {
  ORT_ENFORCE(bg_tasks_.find(run_id) != bg_tasks_.end());
  return bg_tasks_[run_id]->status_future_.get();
}

}  // namespace contrib
}  // namespace onnxruntime