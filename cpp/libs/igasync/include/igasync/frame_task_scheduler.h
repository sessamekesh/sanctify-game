#ifndef _LIB_IGASYNC_FRAME_TASK_SCHEDULER_H_
#define _LIB_IGASYNC_FRAME_TASK_SCHEDULER_H_

#include <igasync/task_list.h>
#include <igcore/vector.h>

/**
 * FrameTaskScheduler - utility class that emits a set of tasks each time it is
 *  invoked (every frame). These tasks can be immediately executed or added to
 *  a task list, the scheduler doesn't care.
 *
 * Example use: a long-running atmosphere simulation wants to generate a huge
 *  2048x2048 skybox texture using high resolution shading, which is expected
 *  to take 2000ms total. In order to avoid dropping 120 frames to generate the
 *  texture, the skybox generator chooses to emit a FrameTaskScheduler that is
 *  pinged once per frame on the main thread, and emits as many 32x32 tiles
 *  as the framework thinks can fit into a 1.5ms window (in this example, 3 such
 *  tiles can be scheduled per frame). These tasks are added to the main thread
 *  task list, where they are executed in the pre-scene-present phase, which has
 *  an 8ms budget. The 8ms is not completely taken up by skybox generation (like
 *  it would be if the skybox was the only producer), and other long-running
 *  expensive tasks are allowed to interleave their partial operations as well.
 */

namespace indigo::core {
class IFrameTaskScheduler {
 public:
  virtual void schedule_frame_tasks(
      std::shared_ptr<core::TaskList> task_list) = 0;

  virtual bool is_finished() = 0;
};

/**
 * Utility class that maintains a group of task schedulers, and erases them when
 *  they have finished performing their tasks
 */
class FrameTaskSchedulerExecutor {
 public:
  FrameTaskSchedulerExecutor(std::shared_ptr<core::TaskList> task_list)
      : schedulers_(2), task_list_(task_list) {}

  void add_scheduler(std::shared_ptr<IFrameTaskScheduler> scheduler) {
    schedulers_.push_back(scheduler);
  }

  void tick() {
    for (int i = 0; i < schedulers_.size(); i++) {
      schedulers_[i]->schedule_frame_tasks(task_list_);

      if (schedulers_[i]->is_finished()) {
        schedulers_.delete_at(i, true);
        i--;
      }
    }
  }

 private:
  core::Vector<std::shared_ptr<IFrameTaskScheduler>> schedulers_;
  std::shared_ptr<core::TaskList> task_list_;
};

}  // namespace indigo::core

#endif
