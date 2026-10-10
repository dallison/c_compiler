// std::string is usable in constant evaluation (transient allocation only).
#include <string>
#include <string_view>

constexpr int appended_size() {
  std::string s = "hello";
  s += " world";
  return static_cast<int>(s.size());
}
static_assert(appended_size() == 11);

constexpr bool compares() {
  std::string a = "abc";
  std::string b = "abd";
  return a < b && a != b && a.compare(b) < 0 && a.find('c') == 2 &&
         a + b == "abcabd" && (a <=> b) < 0;
}
static_assert(compares());

constexpr bool grows_to_heap() {
  std::string s;
  for (int i = 0; i < 40; ++i) {
    s.push_back(static_cast<char>('a' + i % 26));
  }
  std::string copy = s;
  std::string moved = static_cast<std::string&&>(copy);
  return s.size() == 40 && moved == s && copy.empty() && s[27] == 'b' &&
         s.back() == 'n';
}
static_assert(grows_to_heap());

constexpr bool edits() {
  std::string s = "hello, world";
  s.erase(5, 7);
  s.insert(0, 2, '>');
  s.replace(2, 1, 1, 'H');
  s.append(3, '!');
  std::string sub = s.substr(2, 5);
  s.resize(4);
  s.pop_back();
  return sub == "Hello" && s == ">>H" && sub.rfind('l') == 3 &&
         sub.find("llo") == 2 && sub.find_first_of("ol") == 2;
}
static_assert(edits());

constexpr bool assigns_and_views() {
  std::string a = "short";
  std::string b = "a string that is long enough to need the heap";
  a = b;
  b = "x";
  a.swap(b);
  std::string_view view = b;
  return a == "x" && view.size() == 45 && view.substr(0, 8) == "a string";
}
static_assert(assigns_and_views());

int main() {
  if (appended_size() != 11) return 1;
  if (!compares()) return 2;
  if (!grows_to_heap()) return 3;
  if (!edits()) return 4;
  if (!assigns_and_views()) return 5;
  return 0;
}
