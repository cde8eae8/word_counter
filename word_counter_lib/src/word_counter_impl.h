#include <cstddef>
#include <span>
#include <string_view>

#include "include/word_counter.h"
#include "src/executor.h"

namespace word_counter {

template <typename Key> class ViewResult {
public:
  ViewResult() = default;
  ViewResult(const ViewResult &) = delete;
  ViewResult &operator=(const ViewResult &) = delete;
  ViewResult(ViewResult &&) = default;
  ViewResult &operator=(ViewResult &&) = default;

  std::unordered_map<Key, uint64_t> words;
  Key prefix;
  Key suffix;
  // If |has_spaces| is false then |suffix| of normalized ViewResult contains
  // the whole text piece and |prefix| is empty.
  bool has_spaces = false;

  void Normalize() {
    if (!has_spaces) {
      assert(prefix.empty() || suffix.empty());
      if (!prefix.empty()) {
        std::swap(prefix, suffix);
      }
    }
  }
};

Result ResultFromView(ViewResult<std::string> view);

using TaskResult = ViewResult<std::string_view>;

class WordCounterImpl {
public:
  explicit WordCounterImpl(size_t n_threads) : executor_(n_threads - 1) {}

  Result CountWords(std::span<char> text);

private:
  concurrent::ThreadExecutor executor_;
  std::vector<std::future<TaskResult>> results_;
};

std::string_view Merge(std::string_view lhs, std::string_view rhs);
std::string Merge(std::string lhs, std::string_view rhs);

template <typename Key1, typename Key2>
[[nodiscard]] ViewResult<Key1> MergeResults(ViewResult<Key1> &&lhs,
                                            ViewResult<Key2> &&rhs) {
  ViewResult<Key1> result;
  lhs.Normalize();
  rhs.Normalize();
  result.words = std::move(lhs.words);
  for (const auto &entry : rhs.words) {
    result.words[Key1(entry.first)] += entry.second;
  }

  result.has_spaces = lhs.has_spaces || rhs.has_spaces;
  if (!result.has_spaces) {
    result.prefix = "";
    result.suffix = Merge(std::move(lhs.suffix), std::move(rhs.suffix));
    return result;
  }
  result.prefix = lhs.has_spaces
                      ? lhs.prefix
                      : Merge(std::move(lhs.suffix), std::move(rhs.prefix));
  result.suffix = rhs.has_spaces
                      ? rhs.suffix
                      : Merge(std::move(lhs.suffix), std::move(rhs.suffix));
  if (lhs.has_spaces && rhs.has_spaces &&
      lhs.suffix.size() + rhs.prefix.size() != 0) {
    Key1 middle_word = Merge(std::move(lhs.suffix), std::move(rhs.prefix));
    ++result.words[Key1(middle_word)];
  }
  return result;
}
} // namespace word_counter
