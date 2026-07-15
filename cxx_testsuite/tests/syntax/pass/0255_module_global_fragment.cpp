// RUN: -std=c++20
module;
int global_fragment_value = 1;
export module syntax_global;

export int global_fragment_answer() {
  return global_fragment_value;
}
