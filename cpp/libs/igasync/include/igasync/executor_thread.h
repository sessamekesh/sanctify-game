#ifndef _LIB_IGASYNC_EXECUTOR_THREAD_H_
#define _LIB_IGASYNC_EXECUTOR_THREAD_H_

#include <igasync/task_list.h>
#include <igcore/config.h>
#include <igcore/vector.h>
#include <atomic>
#include <condition_variable>
#include <shared_mutex>
#include <thread>

#ifndef IG_ENABLE_THREADS
#error Executor threads are only suppored in multi-threaded Indigo builds!
#endif

namespace indigo::core {
/**
 * ExecutorThread
 *
 * Object that spawns and maintains a thread that attempts to pull operations
 * from attached task lists in a round-robin fashion. The thread may be slept
 * if the underlying task lists are all empty, and awoken via the
 * ITaskScheduledListener interface.
 *
 * Only usable in threaded builds!
 *
 * A quick performance caveat: adding/removing task lists is a mutex-guarded
 *  operation, and should be avoided unless the thread is sleeping and the
 *  task lists in question are empty. These operations are thread-safe, so if
 *  performing them on empty executors and empty task lists is impossible,
 *  you can instead perform those operations on a side thread.
 */

class ExecutorThread : public std::enable_shared_from_this<ExecutorThread>,
                       public ITaskScheduledListener {
 public:
  ExecutorThread();
  ~ExecutorThread();

  ExecutorThread(const ExecutorThread&) = delete;
  ExecutorThread(ExecutorThread&&) = delete;

  void add_task_list(std::shared_ptr<core::TaskList> task_list);
  void remove_task_list(std::shared_ptr<core::TaskList> task_list);
  void clear_all_task_lists();

  // ITaskScheduledListener
  void notify_task_added() override;

 private:
  core::Vector<std::shared_ptr<core::TaskList>> task_lists_;
  std::shared_mutex task_list_m_;
  std::atomic_bool is_cancelled_;

  std::atomic_size_t next_task_list_idx_;

  std::condition_variable has_task_cv_;
  std::mutex has_task_mutex_;
  std::thread t_;
};
}  // namespace indigo

#endif
