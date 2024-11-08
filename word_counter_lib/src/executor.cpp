#include "src/executor.h"
#include <future>

namespace concurrent {

ThreadExecutor::ThreadExecutor(size_t n_threads) : finished_(false) {
  threads_.reserve(n_threads);
  for (size_t i = 0; i < n_threads; ++i) {
    threads_.emplace_back([this, index = i]() {
      int iteration = 1;
      while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        start_cv_.wait(lock, [this, iteration]() {
          return finished_ || iteration == iteration_;
        });
        if (finished_) {
          return;
        }
        ++iteration;
        if (index >= tasks_.size() || !tasks_[index].valid()) {
          continue;
        }
        auto task = std::move(tasks_[index]);
        tasks_[index] = {};
        lock.unlock();
        task();
      }
    });
  }
}

ThreadExecutor::~ThreadExecutor() {
  if (!finished_) {
    Quit();
  }
}

void ThreadExecutor::Quit() {
  {
    std::lock_guard lock(mutex_);
    finished_ = true;
  }
  start_cv_.notify_all();
  for (auto &t : threads_) {
    t.join();
  }
}

size_t ThreadExecutor::GetNumberOfThread() const { return threads_.size(); }

void ThreadExecutor::Run() {
  std::lock_guard lock(mutex_);
  ++iteration_;
  start_cv_.notify_all();
}
} // namespace concurrent