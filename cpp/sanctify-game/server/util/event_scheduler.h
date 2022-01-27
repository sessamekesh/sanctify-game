#ifndef SANCTIFY_GAME_SERVER_UTIL_EVENT_SCHEDULER_H
#define SANCTIFY_GAME_SERVER_UTIL_EVENT_SCHEDULER_H

#include <igasync/task_list.h>

#include <chrono>
#include <queue>
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

  void schedule_task(std::chrono::high_resolution_clock::time_point time_point,
                     std::shared_ptr<indigo::core::TaskList> task_list,
                     std::function<void()> callback);

 private:
  struct ScheduledTask {
    std::chrono::high_resolution_clock::time_point TimePoint;
    std::shared_ptr<indigo::core::TaskList> TaskList;
    std::function<void()> Callback;

    bool operator()(const ScheduledTask& l, const ScheduledTask& r) {
      return l.TimePoint < r.TimePoint;
    }
  };

 private:
  std::thread event_loop_thread_;

  // Doesn't need a lock because it will only be set in one location and only
  // once - race condition basically means "whoops won't stop" for a sec
  bool is_running_;

  std::mutex condvar_lock_;
  std::condition_variable condvar_;

  std::mutex scheduled_tasks_lock_;
  std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, ScheduledTask>
      scheduled_tasks_;
};

}  // namespace sanctify

#endif
