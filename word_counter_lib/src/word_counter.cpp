#include "include/word_counter.h"
#include "src/word_counter_impl.h"

namespace word_counter {
Result Merge(Result &&lhs, Result &&rhs) {
  auto create = [](Result &&result) {
    ViewResult<std::string> view;
    view.prefix = std::move(result.prefix);
    view.suffix = std::move(result.suffix);
    view.words = std::move(result.words);
    view.has_spaces = result.has_spaces;
    return view;
  };
  auto lhs_view = create(std::move(lhs));
  auto rhs_view = create(std::move(rhs));
  auto result_view = MergeResults(std::move(lhs_view), std::move(rhs_view));
  return ResultFromView(std::move(result_view));
}

std::unordered_map<std::string, uint64_t> Finish(Result &&result) {
  if (!result.prefix.empty()) {
    ++result.words[std::move(result.prefix)];
  }
  if (!result.suffix.empty()) {
    ++result.words[std::move(result.suffix)];
  }
  return std::move(result.words);
}

WordCounter::WordCounter(size_t n_threads)
    : impl_(std::make_unique<WordCounterImpl>(n_threads)) {}
WordCounter::~WordCounter() = default;
WordCounter::WordCounter(WordCounter &&) = default;

Result WordCounter::CountWords(std::span<char> text) {
  return impl_->CountWords(text);
}
} // namespace word_counter