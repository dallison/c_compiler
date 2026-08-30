// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <format>
#include <memory_resource>
#include <sstream>
#include <stacktrace>
#include <string>

[[gnu::noinline]] static std::stacktrace capture_leaf() {
  return std::stacktrace::current();
}

// `noinline` keeps the call, but `return capture_leaf();` is still a tail call,
// which leaves this function no frame of its own to appear in the trace.  A
// side effect after the call is what makes the frame outlive it.
static volatile int keep_frame_alive = 0;

[[gnu::noinline]] static std::stacktrace capture_middle() {
  std::stacktrace captured = capture_leaf();
  keep_frame_alive = keep_frame_alive + 1;
  return captured;
}

[[gnu::noinline]] static std::stacktrace capture_limited() {
  return std::stacktrace::current(1, 2);
}

int main() {
  std::stacktrace trace = capture_middle();
  if (trace.size() < 3) {
    return 1;
  }
  if (!trace[0] || trace[0].native_handle() == 0) {
    return 2;
  }
  if (trace[0].description().find("capture_leaf") == std::string::npos) {
    return 3;
  }
  if (trace[1].description().find("capture_middle") == std::string::npos) {
    return 4;
  }
  if (trace[0].source_file().find("0401_standard_stacktrace.cpp") ==
          std::string::npos ||
      trace[0].source_line() == 0) {
    return 5;
  }

  std::stacktrace copy = trace;
  if (!(copy == trace) || (copy <=> trace) != std::strong_ordering::equal) {
    return 6;
  }
  if (std::hash<std::stacktrace>{}(copy) !=
      std::hash<std::stacktrace>{}(trace)) {
    return 7;
  }

  std::stacktrace limited = capture_limited();
  if (limited.size() > 2 || limited.empty()) {
    return 8;
  }
  if (limited[0].description().find("capture_limited") != std::string::npos) {
    return 9;
  }
  std::pmr::stacktrace polymorphic = std::pmr::stacktrace::current(0, 2);
  if (polymorphic.empty() || polymorphic.size() > 2) {
    return 13;
  }
  std::pmr::polymorphic_allocator<std::stacktrace_entry> failing_allocator(
      std::pmr::null_memory_resource());
  std::pmr::stacktrace failed =
      std::pmr::stacktrace::current(0, 2, failing_allocator);
  if (!failed.empty()) {
    return 14;
  }
  if (!std::stacktrace::current(0, 0).empty()) {
    return 15;
  }

  std::string text = std::to_string(trace);
  if (text.find("capture_leaf") == std::string::npos) {
    return 10;
  }
  if (std::format("{}", trace) != text) {
    return 11;
  }
  std::string padded = std::format("{:>80}", trace);
  if (padded.size() < 80 ||
      padded.substr(padded.size() - text.size()) != text) {
    return 16;
  }
  std::string entry_text = trace[0].description();
  if (std::format("{:.3}", trace[0]) != entry_text.substr(0, 3)) {
    return 17;
  }
  std::ostringstream output;
  output << trace;
  if (output.str() != text) {
    return 12;
  }
  std::basic_ostringstream<wchar_t> wide_output;
  wide_output << trace;
  if (wide_output.str().size() != text.size()) {
    return 18;
  }
  return 0;
}
