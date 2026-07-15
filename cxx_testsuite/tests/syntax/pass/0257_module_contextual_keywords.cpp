// RUN: -std=c++20
export module syntax_contextual;

int module;
int import;

export int contextual_keywords() {
  module = 4;
  import = 5;
  return module + import;
}
