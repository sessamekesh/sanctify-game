#include <igasync/task_list.h>

using namespace indigo;
using namespace core;

std::unique_ptr<Task> Task::of(std::function<void()>&& fn) {
  return std::unique_ptr<Task>(new Task{std::move(fn), TaskState::Created});
}

TaskList::TaskList() {}

void TaskList::add_task(std::unique_ptr<Task> task) {
  std::lock_guard l(m_);
  task->State = TaskState::Scheduled;

  task_list_.push_back(std::move(task));
  for (int i = 0; i < listeners_.size(); i++) {
    listeners_[i]->notify_task_added();
  }
}

void TaskList::add_listener(
    const std::shared_ptr<ITaskScheduledListener>& listener) {
  std::lock_guard l(m_);
  listeners_.push_back(listener);
}

void TaskList::remove_listener(
    const std::shared_ptr<ITaskScheduledListener>& listener) {
  std::lock_guard l(m_);
  for (auto it = listeners_.begin(); it != listeners_.end(); it++) {
    if (*it == listener) {
      it = listeners_.erase(it);
    }
  }
}

bool TaskList::execute_next() {
  std::unique_ptr<Task> task;
  {
    std::lock_guard l(m_);
    if (task_list_.size() > 0) {
      task = std::move(task_list_[0]);
      task_list_.erase(task_list_.begin());
    }
  }

  if (task) {
    task->State = TaskState::Started;
    task->Fn();
    task->State = TaskState::Done;
    return true;
  }

  return false;
}