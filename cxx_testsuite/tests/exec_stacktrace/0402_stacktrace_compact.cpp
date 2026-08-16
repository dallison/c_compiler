// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <stacktrace>
#include <string>

[[gnu::noinline]] static std::stacktrace compact_leaf() {
  return std::stacktrace::current(0, 2);
}

[[gnu::noinline]] static std::stacktrace compact_caller() {
  return compact_leaf();
}

int main() {
  std::stacktrace trace = compact_caller();
  if (trace.empty() || trace.size() > 2) {
    return 1;
  }
  if (!trace[0] || trace[0].native_handle() == 0) {
    return 2;
  }
  if (trace[0].description().find("compact_leaf") == std::string::npos) {
    return 3;
  }
  if (trace[0].source_line() == 0) {
    return 5;
  }
  std::string source_file = trace[0].source_file();
  if (source_file.find("0402_stacktrace_compact.cpp") ==
      std::string::npos) {
    return 5;
  }
  if (trace.size() > 1 &&
      trace[1].description().find("compact_caller") == std::string::npos) {
    return 4;
  }
  return 0;
}
