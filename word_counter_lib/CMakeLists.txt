add_library(word_counter_lib 
  include/word_counter.h
  src/word_counter.cpp 
  src/word_counter_impl.h
  src/word_counter_impl.cpp
  src/executor.h
  src/executor.cpp
)
target_include_directories(word_counter_lib PUBLIC include)
target_include_directories(word_counter_lib PRIVATE .)