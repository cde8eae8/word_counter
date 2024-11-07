#include "word_counter.h"

#include <span>
#include <iostream>

int main() {
    std::string text = "hellow wOrld  world how are you ?";
    word_counter::WordCounter counter;
    auto words = counter.CountWords(std::span<char>(text.begin(), text.end()));
    if (!words.prefix.empty()) {
        ++words.words[words.prefix];
    }
    if (!words.suffix.empty()) {
        ++words.words[words.suffix];
    }
    for (const auto& w : words.words) {
        std::cout << w.first << " " << w.second << "\n";
    }
}