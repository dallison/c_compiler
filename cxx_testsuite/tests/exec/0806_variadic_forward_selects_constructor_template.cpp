// RUN: -std=c++17
// EXPECT_EXIT: 0

// `T t(std::forward<Args>(args)...)` in a function template must keep the
// forwarding constructor template deduced for an lvalue argument instead of
// falling back to a same-arity copy constructor.  protobuf's
// `std::make_unique<Value>(x)` with `Value` a variant holding a move-only
// class otherwise instantiated that class's deleted copy constructor.

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct Tracked {
  int value;
  int copies = 0;
  Tracked(const Tracked& other) : value(other.value), copies(other.copies + 1) {}
  Tracked(Tracked&& other) : value(other.value), copies(other.copies) {}
  template <class T>
  Tracked(T&& v) : value(static_cast<int>(v.size())) {}
  Tracked(int& v) : value(v) {}
};

struct Converting {
  int value;
  Converting(const Converting& other) : value(other.value + 100) {}
  Converting(Converting&& other) : value(other.value) {}
  template <class T>
  Converting(T&& v) : value(v) {}
};

class MoveOnly final {
 public:
  using Value = std::variant<int, std::string, MoveOnly, std::vector<MoveOnly>>;
  explicit MoveOnly(int v) : v_(v) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;
  int v_;
};

template <class T, class... Args>
T Direct(Args&&... args) {
  T t(std::forward<Args>(args)...);
  return t;
}

template <class T, class... Args>
std::unique_ptr<T> Make(Args&&... args) {
  return std::unique_ptr<T>(new T{std::forward<Args>(args)...});
}

int main() {
  int x = 5;
  if (Direct<Converting>(x).value != 5) return 1;
  if (Make<Converting>(x)->value != 5) return 2;

  std::string s = "abc";
  const Tracked from_string = Direct<Tracked>(s);
  if (from_string.value != 3) return 3;
  const Tracked copy = Direct<Tracked>(from_string);
  if (copy.value != 3 || copy.copies < 1) return 4;

  auto boxed = std::make_unique<MoveOnly::Value>(x);
  if (std::get<int>(*boxed) != 5) return 5;
  auto text = std::make_unique<MoveOnly::Value>(s);
  if (std::get<std::string>(*text) != "abc") return 6;
  auto nested = std::make_unique<MoveOnly::Value>(MoveOnly(7));
  if (std::get<MoveOnly>(*nested).v_ != 7) return 7;
  return 0;
}
