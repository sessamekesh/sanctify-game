#include <igasync/executor_thread.h>
#include <igcore/log.h>

using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "[ExecutorThread]";
}

ExecutorThread::ExecutorThread()
    : task_lists_(4),
      is_cancelled_(false),
      next_task_list_idx_(0),
      t_([t = this]() {
        auto tid = std::this_thread::get_id();
        core::Logger::log(kLogLabel)
            << "Starting executor thread [[" << tid << "]]";
        while (!t->is_cancelled_) {
          // Execute tasks from the task provider until there are no more to
          // execute...
          while (!t->is_cancelled_) {
            std::shared_lock l(t->task_list_m_);
            bool task_executed = false;
            for (int i = 0; i < t->task_lists_.size(); i++) {
              int idx = (i + t->next_task_list_idx_) % t->task_lists_.size();
              if (t->task_lists_[idx]->execute_next()) {
                t->next_task_list_idx_ = (idx + 1) % t->task_lists_.size();
                task_executed = true;
                break;
              }
            }

            // If no tasks are executed, go out and wait on cond var
            if (!task_executed) {
              break;
            }
          }

          // This thread can rest, since all task lists are empty.
          std::unique_lock l(t->has_task_mutex_);
          t->has_task_cv_.wait(l, [t]() {
            // Predicate is not matched if the task provider is empty, leave and
            // wait
            std::shared_lock l(t->task_list_m_);

            // If the task provider successfully executed a task, stop blocking!
            for (int i = 0; i < t->task_lists_.size(); i++) {
              int idx = (i + t->next_task_list_idx_) % t->task_lists_.size();
              if (t->task_lists_[idx]->execute_next()) {
                t->next_task_list_idx_ = (idx + 1) % t->task_lists_.size();
                return true;
              }
            }

            // No task was executed, but still continue if this thread is
            // shutting down
            return t->is_cancelled_.load();
          });
        }

        core::Logger::log(kLogLabel)
            << "Shutting down executor thread [[" << tid << "]]";
      }) {}

ExecutorThread::~ExecutorThread() {
  clear_all_task_lists();

  is_cancelled_ = true;
  has_task_cv_.notify_all();

  t_.join();
}

void ExecutorThread::add_task_list(std::shared_ptr<core::TaskList> task_list) {
  {
    std::unique_lock l(task_list_m_);
    task_lists_.push_back(task_list);
    task_list->add_listener(shared_from_this());
  }
  has_task_cv_.notify_all();
}

void ExecutorThread::remove_task_list(
    std::shared_ptr<core::TaskList> task_list) {
  {
    std::unique_lock l(task_list_m_);
    task_lists_.erase(task_list, /* preserve_order= */ false);
    task_list->remove_listener(shared_from_this());
  }
  has_task_cv_.notify_all();
}

void ExecutorThread::clear_all_task_lists() {
  {
    std::unique_lock l(task_list_m_);
    for (int i = 0; i < task_lists_.size(); i++) {
      task_lists_[i]->remove_listener(shared_from_this());
    }
    task_lists_.clear();
  }
  has_task_cv_.notify_all();
}

void ExecutorThread::notify_task_added() { has_task_cv_.notify_one(); }