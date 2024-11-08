#include "word_counter.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>

// 32 Mb
static constexpr size_t kBufferSize = 32 * 1024 * 1024;

void PrintUsage(char *program_name, const std::string &message) {
  std::cerr << program_name << " [input_file] [output_file]" << std::endl;
  std::cerr << message << std::endl;
}

std::unordered_map<std::string, uint64_t> CountWords(size_t n_threads,
                                                     std::ifstream &infile) {
  std::vector<char> buffer;
  buffer.resize(kBufferSize);
  word_counter::WordCounter counter(n_threads);
  word_counter::Result result;
  while (infile) {
    infile.read(buffer.data(), buffer.size());
    std::span<char> data(buffer.data(), infile.gcount());
    result = word_counter::Merge(std::move(result), counter.CountWords(data));
  }
  return word_counter::Finish(std::move(result));
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::string message =
        argc < 3 ? "Not enough arguments" : "Too many arguments";
    message += ". Expected two files";
    PrintUsage(argv[0], message);
    return 1;
  }
  std::string in_file = argv[1];
  std::string out_file = argv[2];

  std::ifstream in(in_file);
  if (in.fail()) {
    std::cerr << "Can not open " << in_file << " for reading" << std::endl;
    return 1;
  }
  auto words =
      CountWords(std::max(1u, std::thread::hardware_concurrency()), in);

  std::vector<std::pair<std::string, uint64_t>> words_list;
  words_list.reserve(words.size());
  std::move(words.begin(), words.end(), std::back_inserter(words_list));
  std::sort(words_list.begin(), words_list.end(),
            [](const auto &lhs, const auto &rhs) {
              return std::forward_as_tuple(-lhs.second, lhs.first) <
                     std::forward_as_tuple(-rhs.second, rhs.first);
            });

  std::ofstream out(out_file);
  if (out.fail()) {
    std::cerr << "Can not open " << out_file << " for writing" << std::endl;
    return 1;
  }
  for (const auto &w : words_list) {
    out << w.second << " " << w.first << "\n";
  }
  return 0;
}