// RUN: -std=c++20
// EXPECT_EXIT: 0
// out_of_range on illegal positions, allocator move-assignment, string_view
// iterator construction, and the C++20 string / string_view surface.
#include <functional>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

using std::string;
using std::string_view;

static int fail_at() {
  string s("ab");
  try {
    (void)s.at(2);
    return 1;
  } catch (const std::out_of_range& ex) {
    const char* msg = ex.what();
    if (msg == 0 || msg[0] != 'b') {
      return 2;
    }
  } catch (...) {
    return 3;
  }
  if (s != "ab") {
    return 4;
  }
  try {
    (void)s.substr(3);
    return 5;
  } catch (const std::out_of_range&) {
  } catch (...) {
    return 6;
  }
  try {
    s.insert(3, "z");
    return 7;
  } catch (const std::out_of_range&) {
  } catch (...) {
    return 8;
  }
  try {
    s.erase(3);
    return 9;
  } catch (const std::out_of_range&) {
  } catch (...) {
    return 10;
  }
  try {
    s.replace(4, 1, "q");
    return 11;
  } catch (const std::out_of_range&) {
  } catch (...) {
    return 12;
  }
  if (s != "ab") {
    return 13;
  }
  char buf[4] = {};
  if (s.copy(buf, 1, 1) != 1 || buf[0] != 'b') {
    return 14;
  }
  try {
    (void)s.copy(buf, 1, 3);
    return 15;
  } catch (const std::out_of_range&) {
  } catch (...) {
    return 16;
  }
  return 0;
}

static int fail_view() {
  const char* text = "hello";
  string_view view(text, text + 5);
  if (view.size() != 5 || view[0] != 'h' || view[4] != 'o') {
    return 20;
  }
  if (view.find("ellxyz", 0, 3) != 1) {
    return 21;
  }
  if (view.find_first_of("xyz", 0, 1) != string_view::npos) {
    return 22;
  }
  if (view.find_first_of("xoz", 0, 3) != 4) {
    return 23;
  }
  if (view.compare(1, 2, "elXYZ", 2) != 0) {
    return 24;
  }
  if (view.compare(0, 5, text + 0) != 0) {
    return 25;
  }
  if (std::to_address(text) != text) {
    return 26;
  }
  static_assert(std::ranges::enable_borrowed_range<string_view>);
  static_assert(std::ranges::enable_view<string_view>);
  string_view empty;
  if (std::hash<string_view>{}(empty) !=
      std::hash<std::u16string_view>{}(std::u16string_view())) {
    return 29;
  }
  std::u16string_view wide(u"ab");
  if (std::hash<std::u16string_view>{}(wide) == std::hash<string_view>{}("ab")) {
    return 27;
  }
  if (std::hash<std::u16string_view>{}(wide) !=
      std::hash<std::u16string_view>{}(std::u16string_view(u"ab"))) {
    return 28;
  }
  return 0;
}

static int fail_assign_and_concat() {
  string s;
  s = string_view("hi");
  if (s != "hi" || s.find(string_view("i")) != 1) {
    return 30;
  }
  s.assign(string_view("abcdef"), 2, 3);
  if (s != "cde") {
    return 31;
  }
  string letters{'a', 'b', 'c'};
  if (letters != "abc") {
    return 32;
  }
  if (('x' + string("y")) != "xy") {
    return 33;
  }
  if ((string("a") + 'b') != "ab") {
    return 34;
  }
  string heap("0123456789abcdef");
  string moved = std::move(heap) + string("Z");
  if (moved != "0123456789abcdefZ") {
    return 35;
  }
  using namespace std::string_literals;
  string lit = "ab"s;
  if (lit != "ab" || lit.size() != 2) {
    return 36;
  }
  if (std::hash<string>{}(lit) != std::hash<string_view>{}(string_view("ab"))) {
    return 37;
  }
  return 0;
}

struct TrackedAlloc {
  using value_type = char;
  using propagate_on_container_move_assignment = std::false_type;
  using is_always_equal = std::false_type;
  int id;
  explicit TrackedAlloc(int i) : id(i) {}
  char* allocate(std::size_t n) {
    return static_cast<char*>(::operator new(n));
  }
  void deallocate(char* p, std::size_t) { ::operator delete(p); }
  bool operator==(const TrackedAlloc& other) const { return id == other.id; }
  bool operator!=(const TrackedAlloc& other) const { return id != other.id; }
};

static int fail_alloc_move() {
  using S = std::basic_string<char, std::char_traits<char>, TrackedAlloc>;
  const char* long_text = "0123456789abcdef";
  S source(long_text, TrackedAlloc(1));
  S different(TrackedAlloc(2));
  different = std::move(source);
  if (different.size() != 16 || different != long_text) {
    return 40;
  }
  if (different.get_allocator().id != 2) {
    return 41;
  }
  if (source.size() != 16 || source != long_text) {
    return 42;
  }
  if (source.get_allocator().id != 1) {
    return 43;
  }

  S same_src(long_text, TrackedAlloc(7));
  S same_dst(TrackedAlloc(7));
  same_dst = std::move(same_src);
  if (same_dst != long_text || same_dst.get_allocator().id != 7) {
    return 44;
  }
  if (!same_src.empty()) {
    return 45;
  }

  string ordinary("0123456789abcdef");
  string stolen = std::move(ordinary);
  if (stolen != "0123456789abcdef" || !ordinary.empty()) {
    return 46;
  }
  return 0;
}

int main() {
  int result = fail_at();
  if (result != 0) {
    return result;
  }
  result = fail_view();
  if (result != 0) {
    return result;
  }
  result = fail_assign_and_concat();
  if (result != 0) {
    return result;
  }
  return fail_alloc_move();
}
