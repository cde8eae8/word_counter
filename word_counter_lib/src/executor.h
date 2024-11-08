#include <cassert>
#include <future>
#include <vector>

namespace concurrent {

class ThreadExecutor {
public:
  ThreadExecutor(size_t n_threads);
  ThreadExecutor(const ThreadExecutor &) = delete;
  ThreadExecutor &operator=(const ThreadExecutor &) = delete;
  ~ThreadExecutor();

  template <typename T>
  std::vector<std::future<T>>
  SetTasks(std::vector<std::packaged_task<T()>> tasks) {
    assert(!finished_);
    std::lock_guard lock(mutex_);
    assert(threads_.size() == tasks.size());
    std::vector<std::future<T>> results;
    results.reserve(tasks.size());
    tasks_.clear();
    tasks_.reserve(tasks.size());
    for (auto &task : tasks) {
      results.emplace_back(task.get_future());
      tasks_.emplace_back(std::packaged_task<void()>(std::move(task)));
      assert(results.back().valid());
      assert(tasks_.back().valid());
    }
    return results;
  }

  size_t GetNumberOfThread() const;

  void Run();

  void Quit();

private:
  size_t iteration_ = 0;
  std::mutex mutex_;
  std::condition_variable start_cv_;
  std::vector<std::thread> threads_;
  std::vector<std::packaged_task<void()>> tasks_;
  bool finished_;
};

} // namespace concurrent