#ifndef WORD_COUNTER_H_
#define WORD_COUNTER_H_

#include <span>
#include <string>
#include <unordered_map>

namespace word_counter {
class WordCounterImpl;

// Result of text piece processing.
struct Result {
  // Prefix of the text, it might be suffix of a word
  std::string prefix;

  // Suffix of the text, it might be prefix of a word
  std::string suffix;

  std::unordered_map<std::string, uint64_t> words;

  // True if the original text did not contain spaces.
  // This field is necessary for proper merging of results.
  bool has_spaces;
};

// Merges two results that represent consecutive text pieces.
Result Merge(Result &&lhs, Result &&rhs);
// Returns number of words as if |result| represents the whole text
std::unordered_map<std::string, uint64_t> Finish(Result &&result);

class WordCounter {
public:
  WordCounter(size_t n_threads);
  ~WordCounter();
  WordCounter(const WordCounter &&) = delete;
  WordCounter &operator=(const WordCounter &&) = delete;
  WordCounter(WordCounter &&);
  WordCounter &operator=(WordCounter &&) = delete;

  // NB: text might be modified
  Result CountWords(std::span<char> text);

private:
  std::unique_ptr<WordCounterImpl> impl_;
};
} // namespace word_counter

#endif // WORD_COUNTER_H_