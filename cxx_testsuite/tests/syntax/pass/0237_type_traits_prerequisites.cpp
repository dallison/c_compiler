// RUN: -std=c++20
#include <functional>
#include <type_traits>
#include <utility>

struct DefaultOnly {
  int value;
  DefaultOnly() : value(0) {}
};

struct IntConstruct {
  int value;
  explicit IntConstruct(int initial) : value(initial) {}
};

struct DeletedCopy {
  DeletedCopy() {}
  DeletedCopy(const DeletedCopy&) = delete;
  DeletedCopy& operator=(const DeletedCopy&) = delete;
};

struct PrivateBase {
 private:
  int hidden;
};

struct PublicBase {
  int value;
};

struct PublicDerived : PublicBase {
  int extra;
};

struct AmbiguousLeft : PublicBase {
};
struct AmbiguousRight : PublicBase {
};
struct AmbiguousDerived : AmbiguousLeft, AmbiguousRight {
};

struct NonSwappable {
  NonSwappable() {}
  NonSwappable(const NonSwappable&) = delete;
  NonSwappable& operator=(const NonSwappable&) = delete;
};

struct Callable {
  int operator()(int value) const { return value + 1; }
};

static_assert(std::is_default_constructible<DefaultOnly>::value,
              "default constructible");
static_assert(!std::is_default_constructible<IntConstruct>::value,
              "no default constructor");
static_assert(std::is_constructible<IntConstruct, int>::value,
              "int constructible");
static_assert(!std::is_constructible<IntConstruct>::value,
              "not default constructible");
static_assert(std::is_copy_constructible<int>::value, "int copy constructible");
static_assert(!std::is_copy_constructible<DeletedCopy>::value,
              "deleted copy constructor");
static_assert(std::is_move_constructible<int>::value, "int move constructible");
static_assert(std::is_convertible<int, long>::value, "int to long");
static_assert(!std::is_convertible<IntConstruct, int>::value,
              "no implicit explicit conversion");
static_assert(std::is_assignable<int&, int>::value, "int assignable");
static_assert(!std::is_copy_assignable<DeletedCopy>::value,
              "deleted copy assignment");
static_assert(std::is_destructible<int>::value, "int destructible");
static_assert(std::is_destructible<DefaultOnly>::value,
              "class destructible");
static_assert(std::is_swappable<int>::value, "int swappable");
static_assert(!std::is_swappable<NonSwappable>::value,
              "non-swappable type");
static_assert(std::is_signed<int>::value, "int signed");
static_assert(std::is_unsigned<unsigned int>::value, "unsigned int");
static_assert(!std::is_signed<unsigned int>::value, "unsigned not signed");
static_assert(std::is_base_of<PublicBase, PublicDerived>::value,
              "public inheritance");
static_assert(!std::is_base_of<PrivateBase, int>::value,
              "non-class base");
static_assert(std::is_base_of<PublicBase, AmbiguousDerived>::value,
              "ambiguous base is still a base");
static_assert(std::is_class<PublicDerived>::value, "class type");
static_assert(!std::is_class<int>::value, "int not class");
static_assert((std::is_same<typename std::common_type<int, long>::type, long>::value),
              "common_type int/long");
static_assert((std::is_same<std::common_reference_t<int&, const int&>,
                            const int&>::value),
              "common_reference cvrefs");
static_assert(std::is_invocable<Callable, int>::value, "callable invocable");
static_assert(
    std::is_same<std::invoke_result_t<Callable, int>, int>::value,
    "invoke_result_t");

int main() {
  int seven = 7;
  std::reference_wrapper<int> wrapped = std::ref(seven);
  if (wrapped.get() != 7) {
    return 1;
  }
  Callable callable;
  if (std::invoke(callable, 3) != 4) {
    return 2;
  }
  return 0;
}
