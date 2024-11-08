#include "src/word_counter_impl.h"
#include <future>
#include <iostream>
#include <sstream>

namespace word_counter {
namespace {
template <typename Key>
static ViewResult<Key> CountWordsSingleThread(std::span<char> text) {
  size_t word_start = 0;
  size_t word_size = 0;
  ViewResult<Key> result{};
  for (size_t i = 0; i < text.size(); ++i) {
    char &c = text[i];
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
        ++result.words[Key(word)];
      }
      word_size = 0;
    }
  }
  result.has_spaces = !(word_start == 0 && word_size == text.size());
  if (word_size != 0) {
    result.suffix = {&text[word_start], word_size};
  }
  return result;
}

template <typename Key>
ViewResult<Key>
CountWordsHelper(size_t index, std::span<char> text,
                 std::vector<std::future<TaskResult>> *results) {
  auto result = CountWordsSingleThread<Key>(text);
  uint32_t level = 2;
  while (index % level == 0) {
    auto other_index = index + level / 2;
    if (other_index >= results->size()) {
      break;
    }
    std::stringstream ss;
    result = MergeResults(std::move(result),
                          std::move(results->at(other_index).get()));
    level *= 2;
  }
  return result;
}

std::vector<std::packaged_task<TaskResult()>>
CreateTasks(std::span<char> text, size_t n_chunks,
            std::vector<std::future<TaskResult>> *results, size_t start_index) {
  std::vector<std::packaged_task<TaskResult()>> tasks;
  tasks.reserve(n_chunks);
  size_t chunk_size = 1u + text.size() / n_chunks;
  for (size_t i = 0; i < n_chunks; ++i) {
    size_t chunk_start = std::min(chunk_size * i, text.size());
    size_t chunk_end = std::min(chunk_start + chunk_size, text.size());
    auto chunk = text.subspan(chunk_start, chunk_end - chunk_start);
    tasks.emplace_back([index = i + start_index, chunk, results]() {
      return CountWordsHelper<std::string_view>(index, chunk, results);
    });
  }
  return tasks;
}
} // namespace

Result ResultFromView(ViewResult<std::string> view) {
  Result result;
  result.prefix = std::move(view.prefix);
  result.suffix = std::move(view.suffix);
  result.words = std::move(view.words);
  result.has_spaces = view.has_spaces;
  return result;
}

std::string_view Merge(std::string_view lhs, std::string_view rhs) {
  if (lhs.empty()) {
    return rhs;
  }
  if (rhs.empty()) {
    return lhs;
  }
  assert(rhs.data() == lhs.data() + lhs.size());
  return std::string_view(lhs.data(), lhs.size() + rhs.size());
}

std::string Merge(std::string lhs, std::string_view rhs) {
  lhs += rhs;
  return lhs;
}

Result WordCounterImpl::CountWords(std::span<char> text) {
  if (text.empty()) {
    return {};
  }
  std::vector<std::packaged_task<TaskResult()>> tasks;
  size_t n_helpers = executor_.GetNumberOfThread();
  tasks.reserve(n_helpers);
  uint32_t n_chunks = n_helpers + 1;
  uint32_t chunk_size = 1 + text.size() / n_chunks;
  results_ = executor_.SetTasks(
      CreateTasks(text.subspan(chunk_size), n_helpers, &results_, 1));
  // Add fake first element to fix indices
  results_.insert(results_.begin(), std::future<TaskResult>());
  executor_.Run();
  auto result =
      CountWordsHelper<std::string>(0, text.subspan(0, chunk_size), &results_);
  return ResultFromView(std::move(result));
}
} // namespace word_counter