#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <span>

#include "word_counter.h"

namespace word_counter {

template <typename Key>
class CounterThread {
  public:
  class ViewResult {
   public:
    std::string_view prefix;
    WordCountMap<Key> words;
    std::string_view  suffix;
  };

  struct CommonData {
    std::vector<std::future<ViewResult>> results;
  };

  ViewResult CountWords(std::span<char> text) {
    auto result = CountWordsSingleThread(text);
    uint32_t level = 2;
    while (index_ % level == 0) {
      auto other_index = index_ + level / 2;
      MergeResults(&result, std::move(common_data_.results[other_index].get()));
    }
    return std::move(result);
  }

  // TODO: test on words where word_start = 0, word_size = text.size
  static ViewResult CountWordsSingleThread(std::span<char> text) {
    size_t word_start = 0;
    size_t word_size = 0;
    ViewResult result;
    for (size_t i = 0; i < text.size(); ++i) {
      char& c = text[i];
      if ('A' <= c && c <= 'Z') {
        c = c - 'A' + 'a';
      }
      bool is_letter = ('a' <= c && c <= 'z');
      if (is_letter) {
        if (word_size == 0) {
          word_start = i;
        }
        ++word_size;
      } else if (word_size != 0) {
        std::string_view word = {&text[word_start], word_size};
        if (word_start == 0) {
          result.prefix = word;
        } else {
          auto it = result.words.find(word);
          if (it == result.words.end()) {
            result.words.emplace_hint(it, Key(word), 1);
          } else {
            ++it->second;
          }
        }
        word_size = 0;
      }
    }
    if (word_size != 0) {
      result.suffix = {&text[word_start], word_size};
    }
    return result;
  }

  template <typename U>
  void MergeResults(ViewResult* target, CounterThread<U>::ViewResult&& other) {
    for (auto entry : other) {
      auto it = target->find(entry.first);
      if (it == target->end()) {
        target->words.emplace_hint(it, Key(it.first), 1);
      } else {
        ++it->second;
      }
    }
    target->suffix = Key(other.suffix);
  }

  size_t index_;
  CommonData common_data_;
};

using TaskResult = CounterThread<std::string_view>::ViewResult;

class ThreadExecutor {
  ThreadExecutor(size_t n_threads) : finished_(false) {
    --n_threads;
    tasks_.resize(n_threads);
    for (size_t i = 0; i < n_threads; ++i) {
      tasks_.emplace_back([this, i]() {
        while (true) {
          std::unique_lock<std::mutex> lock(mutex_);
          start_cv_.wait(lock, [this, index = i]() {
            // TODO: is it correct?
            return finished_ || tasks_[index].valid();
          });
          if (finished_) {
            return;
          }
          lock.unlock();
          tasks_[i]();
        }
      });
    }
  }

  std::vector<std::future<TaskResult>> SetTasks(std::vector<std::packaged_task<TaskResult()>> tasks) {
    std::lock_guard lock(mutex_);
    assert(tasks_.size() == tasks.size());
    tasks_ = std::move(tasks);
    std::vector<std::future<TaskResult>> results;
    results.reserve(tasks_.size());
    for (auto& task : tasks_) {
      results.emplace_back(task.get_future());
    }
    return results;
  }

  size_t GetNumberOfThread() const { return threads_.size(); }

  void Run() {
    start_cv_.notify_all();
  }

  void Quit() {
    {
      std::lock_guard lock(mutex_);
      finished_ = true;
    }
    for (auto& t : threads_) {
      t.join();
    }
  }

  ~ThreadExecutor() { if (!finished_) { Quit(); } }

 private:
  std::mutex mutex_;
  std::condition_variable start_cv_;
  std::vector<std::thread> threads_;
  bool finished_;
  std::vector<std::packaged_task<TaskResult()>> tasks_;
};

class WordCounterImpl {
 public:
  explicit WordCounterImpl(size_t n_threads) {
    --n_threads;
    common_data_.results.resize(n_threads);
    for (size_t i = 0; i < n_threads; ++i) {
      threads_.emplace_back([this, i]() {
        std::unique_lock<std::mutex> lock(barrier_mutex_);
        barrier_cv_.wait(lock, [this, index = i]() {
          // TODO: is it correct?
          return common_data_.results[index].valid();
        });
      });
      // TODO: join threads in dctor(?)
    }
  }
  WordCounter::Result CountWords(std::span<char> text) {
    // TODO: use hardware 
    impls_.resize(threads_.size());
    tasks_.reserve(threads_.size());
    {
      std::lock_guard lock(barrier_mutex_);
      for (size_t i = 0; i < impls_.size(); ++i) {
        decltype(tasks_)::value_type task = [this, i, text]() {
          return impls_[i].CountWords(text.subspan(0, 0));
        };
        common_data_.results[i] = task.get_future();
        tasks_.emplace_back(std::move(task));
      }
    }
    barrier_cv_.notify_all();
    auto result = main_impl_.CountWords(text.subspan(0, 0));
    WordCounter::Result r;
    r.prefix = std::move(result.prefix);
    r.words = std::move(result.words);
    r.suffix = std::move(result.suffix);
  }

 private:
  std::vector<std::thread> threads_;
  CounterThread<std::string> main_impl_;
  std::vector<CounterThread<std::string_view>> impls_;
  std::vector<Task> tasks_;
  std::mutex barrier_mutex_;
  std::condition_variable barrier_cv_;
  CounterThread<std::string_view>::CommonData common_data_;
};

WordCounter::WordCounter() : impl_(std::make_unique<WordCounterImpl>()) {}
WordCounter::~WordCounter() = default;
WordCounter::WordCounter(WordCounter&&) = default;

WordCounter::Result WordCounter::CountWords(std::span<char> text) {
  return impl_->CountWords(text);
}

}