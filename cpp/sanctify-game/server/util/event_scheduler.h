#ifndef SANCTIFY_GAME_SERVER_UTIL_EVENT_SCHEDULER_H
#define SANCTIFY_GAME_SERVER_UTIL_EVENT_SCHEDULER_H

#include <igasync/task_list.h>

#include <chrono>
#include <condition_variable>
#include <set>
#include <thread>

namespace sanctify {

/**
 * Event scheduler class - starts a thread that runs in a loop with a condition
 * variable and schedules tasks after a set timeout.
 *
 * Scheduling tasks should be very light weight
 */
class EventScheduler {
 public:
  EventScheduler();
  ~EventScheduler();

  uint32_t schedule_task(
      std::chrono::high_resolution_clock::time_point time_point,
      std::shared_ptr<indigo::core::TaskList> task_list,
      std::function<void()> callback);

  void cancel_task(uint32_t id);

 public:
  struct ScheduledTask {
    uint32_t TaskId;
    std::chrono::high_resolution_clock::time_point TimePoint;
    std::shared_ptr<indigo::core::TaskList> TaskList;
    std::function<void()> Callback;

    // wonky ass shit
    bool operator()(const ScheduledTask& l, const ScheduledTask& r) const {
      return l.TimePoint < r.TimePoint;
    }

    ScheduledTask() = default;
    ScheduledTask(uint32_t task_id,
                  std::chrono::high_resolution_clock::time_point tp,
                  std::shared_ptr<indigo::core::TaskList> tl,
                  std::function<void()> cb)
        : TaskId(task_id), TimePoint(tp), TaskList(tl), Callback(cb) {}

    ScheduledTask(const ScheduledTask&) = default;
    ScheduledTask& operator=(const ScheduledTask&) = default;
  };

 private:
  std::thread event_loop_thread_;

  // Doesn't need a lock because it will only be set in one location and only
  // once - race condition basically means "whoops won't stop" for a sec
  bool is_running_;

  std::mutex condvar_lock_;
  std::condition_variable condvar_;

  std::mutex scheduled_tasks_lock_;
  uint32_t next_id_;
  std::set<ScheduledTask, ScheduledTask> scheduled_tasks_;  // wonky ass shit
};

}  // namespace sanctify

#endif
