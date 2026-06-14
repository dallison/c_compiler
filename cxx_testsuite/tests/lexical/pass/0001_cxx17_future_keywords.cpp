// RUN: -std=c++17
int concept;
int module;
int import;
int char8_t;

int main(void) {
  concept = 1;
  module = 2;
  import = 3;
  char8_t = 4;
  return concept + module + import + char8_t;
}
