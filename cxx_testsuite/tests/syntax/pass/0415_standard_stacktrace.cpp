// RUN: -std=c++23

#include <format>
#include <memory_resource>
#include <stacktrace>
#include <type_traits>
#include <version>

#if __cpp_lib_stacktrace != 202011L
#error "__cpp_lib_stacktrace has the wrong value"
#endif

static_assert(std::is_same_v<std::stacktrace::value_type,
                             std::stacktrace_entry>);
static_assert(std::is_same_v<std::stacktrace::iterator,
                             std::stacktrace::const_iterator>);
static_assert(std::is_same_v<std::stacktrace_entry::native_handle_type,
                             uintptr_t>);
static_assert(std::is_same_v<
              std::pmr::stacktrace::allocator_type,
              std::pmr::polymorphic_allocator<std::stacktrace_entry>>);
static_assert(noexcept(std::stacktrace::current()));

constexpr bool empty_entry_properties() {
  std::stacktrace_entry entry;
  return !entry && entry.native_handle() == 0 &&
         entry == std::stacktrace_entry{};
}

static_assert(empty_entry_properties());

void use_stacktrace_api() {
  std::stacktrace trace = std::stacktrace::current(1, 4);
  std::stacktrace copy(trace);
  std::stacktrace moved(static_cast<std::stacktrace&&>(copy));
  (void)moved.get_allocator();
  (void)moved.begin();
  (void)moved.end();
  (void)moved.rbegin();
  (void)moved.rend();
  (void)moved.empty();
  (void)moved.size();
  (void)moved.max_size();
  if (!moved.empty()) {
    (void)moved[0].description();
    (void)moved.at(0).source_file();
    (void)moved[0].source_line();
    (void)std::hash<std::stacktrace_entry>{}(moved[0]);
  }
  (void)std::hash<std::stacktrace>{}(moved);
  (void)std::to_string(moved);
  (void)std::format("{}", moved);
}
