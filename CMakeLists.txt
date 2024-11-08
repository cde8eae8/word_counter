cmake_minimum_required(VERSION 3.15)

project(
  WordCounter
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(word_counter_lib)

add_executable(counter main.cpp)

target_link_libraries(counter PRIVATE word_counter_lib)