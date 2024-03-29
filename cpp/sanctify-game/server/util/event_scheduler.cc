#include <igcore/log.h>
#include <util/event_scheduler.h>

#include <chrono>

using namespace indigo;
using namespace core;
using namespace sanctify;
using namespace std::chrono_literals;

namespace {
const char* kLogLabel = "EventScheduler";
}

EventScheduler::EventScheduler() : is_running_(true), next_id_(0ul) {
  // Naked this pass - destructor must join thread before closing
  event_loop_thread_ = std::thread([this]() {
    while (is_running_) {
      std::chrono::high_resolution_clock::time_point now =
          std::chrono::high_resolution_clock::now();

      // Flush the queue of expired/ready tasks
      while (true) {
        ScheduledTask next_task;
        {
          std::lock_guard<std::mutex> l(scheduled_tasks_lock_);

          auto next_task_it = scheduled_tasks_.begin();
          if (next_task_it == scheduled_tasks_.end()) {
            break;
          }
          if (next_task_it->TimePoint > now) {
            break;
          }
          next_task = *next_task_it;
          scheduled_tasks_.erase(next_task_it);
        }

        next_task.TaskList->add_task(Task::of(std::move(next_task.Callback)));
      }

      // Decide how long to wait
      std::chrono::high_resolution_clock::time_point wait_until_point =
          now + 25ms;
      {
        std::lock_guard<std::mutex> l(scheduled_tasks_lock_);
        auto next_task_it = scheduled_tasks_.begin();
        if (next_task_it != scheduled_tasks_.end()) {
          if (next_task_it->TimePoint < wait_until_point) {
            wait_until_point = next_task_it->TimePoint;
          }
        }
      }

      // ... And wait until the condvar is awoken, or until the time has passed.
      std::unique_lock cvl(condvar_lock_);
      condvar_.wait_until(cvl, wait_until_point);
    }

    std::lock_guard<std::mutex> l(scheduled_tasks_lock_);
    if (scheduled_tasks_.size() > 0) {
      core::Logger::err(kLogLabel)
          << "Shutdown initiated with " << scheduled_tasks_.size()
          << " unfinished tasks on the queue!";
    }
  });
}

void EventScheduler::cancel_task(uint32_t id) {
  std::lock_guard<std::mutex> l(scheduled_tasks_lock_);
  for (auto it = scheduled_tasks_.begin(); it != scheduled_tasks_.end(); it++) {
    if (it->TaskId == id) {
      it = scheduled_tasks_.erase(it);
    }

    if (it == scheduled_tasks_.end()) {
      return;
    }
  }
}

EventScheduler::~EventScheduler() {
  is_running_ = false;
  condvar_.notify_one();
  event_loop_thread_.join();
}

uint32_t EventScheduler::schedule_task(
    std::chrono::high_resolution_clock::time_point time_point,
    std::shared_ptr<indigo::core::TaskList> task_list,
    std::function<void()> callback) {
  std::lock_guard<std::mutex> l(scheduled_tasks_lock_);
  uint32_t task_id = next_id_++;
  scheduled_tasks_.emplace(
      ScheduledTask{task_id, time_point, task_list, callback});
  return task_id;
}
