#ifndef _LIB_IGASYNC_TASK_LIST_H_
#define _LIB_IGASYNC_TASK_LIST_H_

#include <igcore/config.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace indigo::core {

enum class TaskState {
  Created,
  Scheduled,
  Started,
  Done,
};

struct Task {
  std::function<void()> Fn;
  TaskState State;

  static std::unique_ptr<Task> of(std::function<void()>&& fn);
};

class ITaskScheduledListener {
 public:
  virtual void notify_task_added() = 0;
};

class TaskList {
 public:
  TaskList();

  void add_task(std::unique_ptr<Task> task);

  void add_listener(const std::shared_ptr<ITaskScheduledListener>& listener);
  void remove_listener(const std::shared_ptr<ITaskScheduledListener>& listener);

  // Execute a task, return true if a task was executed (false otherwise)
  bool execute_next();

 private:
  std::vector<std::shared_ptr<ITaskScheduledListener>> listeners_;

  std::mutex m_;
  std::vector<std::unique_ptr<Task>> task_list_;
};

}  // namespace indigo::core

#endif
