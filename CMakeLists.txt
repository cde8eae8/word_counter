cmake_minimum_required(VERSION 3.15)

project(
  WordCounter
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(word_counter_lib word_counter.cpp word_counter.h)

add_executable(word_counter main.cpp)

# Make sure you link your targets with this command. It can also link libraries and
# even flags, so linking a target that does not exist will not give a configure-time error.
target_link_libraries(word_counter PRIVATE word_counter_lib)