#ifndef WORD_COUNTER_H_
#define WORD_COUNTER_H_

#include <unordered_map>
#include <string>
#include <span>

namespace word_counter {
namespace internal {
struct string_hash {
  using is_transparent = void;
  [[nodiscard]] size_t operator()(std::string_view txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(const std::string &txt) const {
    return std::hash<std::string_view>{}(std::string_view(txt));
  }
};

}

template <typename StringT>
using WordCountMap = std::unordered_map<StringT, uint64_t, internal::string_hash, std::equal_to<>>;

class WordCounterImpl;

class WordCounter {
 public:
    struct Result {
        std::string prefix;
        std::string suffix;
        WordCountMap<std::string> words;
    };

    WordCounter();
    ~WordCounter();
    WordCounter(const WordCounter&&) = delete;
    WordCounter& operator=(const WordCounter&&) = delete;
    WordCounter(WordCounter&&);
    WordCounter& operator=(WordCounter&&) = delete;

    Result CountWords(std::span<char> text); 
 private:
    std::unique_ptr<WordCounterImpl> impl_;
};
}

#endif // WORD_COUNTER_H_