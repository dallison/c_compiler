// `return local;` treats the local as an rvalue even when a converting
// constructor of a different class initializes the result.
#include <memory>
#include <type_traits>
#include <utility>

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&& other) : value(other.value) { other.value = -1; }
  int value = 7;
};

template <typename T>
struct Plain {
  template <typename U>
  Plain(U&& u) : value(std::forward<U>(u)) {}
  T value;
};

template <typename T>
struct Result {
  Result(int) : ok(false) {}
  template <typename U = T,
            typename std::enable_if<
                std::is_constructible<T, U&&>::value &&
                    !std::is_same<typename std::decay<U>::type, Result>::value,
                int>::type = 0>
  Result(U&& u) : value(std::forward<U>(u)), ok(true) {}
  T value;
  bool ok;
};

struct Base {
  virtual ~Base() = default;
  virtual int Id() const { return 1; }
};
struct Derived : Base {
  int Id() const override { return 2; }
};

Plain<MoveOnly> MakePlain() {
  MoveOnly m;
  return m;
}

Result<MoveOnly> MakeResult(bool fail) {
  MoveOnly m;
  m.value = 9;
  if (fail) return 0;
  return m;
}

Result<MoveOnly> FromParameter(MoveOnly m) { return m; }

std::unique_ptr<Base> MakeBase() {
  std::unique_ptr<Derived> derived(new Derived);
  return derived;
}

int main() {
  if (MakePlain().value.value != 7) return 1;
  Result<MoveOnly> ok = MakeResult(false);
  if (!ok.ok || ok.value.value != 9) return 2;
  if (MakeResult(true).ok) return 3;
  MoveOnly source;
  source.value = 4;
  Result<MoveOnly> moved = FromParameter(std::move(source));
  if (!moved.ok || moved.value.value != 4 || source.value != -1) return 4;
  std::unique_ptr<Base> base = MakeBase();
  if (!base || base->Id() != 2) return 5;
  return 0;
}
