// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <utility>
#include <new>

struct Source {
  int value;
  explicit Source(int v) : value(v) {}
};

struct Target {
  int category;
  int value;

  Target(Source& source, int extra) : category(1), value(source.value + extra) {}
  Target(Source&& source, int extra) : category(2), value(source.value + extra) {
    source.value = -1;
  }
};

template <class Tag>
struct Maker {
  template <class... Args>
  Target make(Args&&... args) {
    Target result(std::forward<Args>(args)...);
    return result;
  }

  template <class... Args>
  static Target make_static(Args&&... args) {
    Target result(std::forward<Args>(args)...);
    return result;
  }

  template <class U, class... Args>
  static void construct(U* ptr, Args&&... args) {
    new (ptr) U(std::forward<Args>(args)...);
  }

  template <class U, class... Args>
  void construct_member(U* ptr, Args&&... args) {
    new (ptr) U(std::forward<Args>(args)...);
  }
};

template <class T>
struct AllocLike {
  void construct(T* ptr, const T& value) {
    new (ptr) T(value);
  }

  void construct(T* ptr, T&& value) {
    new (ptr) T(static_cast<T&&>(value));
  }

  template <class U, class... Args>
  void construct(U* ptr, Args&&... args) {
    new (ptr) U(std::forward<Args>(args)...);
  }
};

template <class Alloc>
struct TraitsLike;

template <class T>
struct TraitsLike<AllocLike<T> > {
  using value_type = T;

  static void construct(Target* ptr) {
    new (ptr) Target(Source(1), 2);
  }

  static void construct(Target* ptr, const Target& value) {
    new (ptr) Target(value);
  }

  static void construct(Target* ptr, Target&& value) {
    new (ptr) Target(static_cast<Target&&>(value));
  }

  template <class U, class... Args>
  static void construct(U* ptr, Args&&... args) {
    new (ptr) U(std::forward<Args>(args)...);
  }

  template <class U, class... Args>
  static void construct_with_alloc(AllocLike<T>& alloc, U* ptr, Args&&... args) {
    alloc.construct(ptr, std::forward<Args>(args)...);
  }
};

int main(void) {
  Maker<int> maker;

  Source lvalue(10);
  Target first = maker.make(lvalue, 4);
  if (first.category != 1 || first.value != 14 || lvalue.value != 10) {
    return 1;
  }

  Source rvalue(20);
  Target second = maker.make(std::move(rvalue), 5);
  if (second.category != 2 || second.value != 25 || rvalue.value != -1) {
    return 2;
  }

  Source static_lvalue(30);
  Target third = Maker<int>::make_static(static_lvalue, 6);
  if (third.category != 1 || third.value != 36 || static_lvalue.value != 30) {
    return 3;
  }

  Source static_rvalue(40);
  Target fourth = Maker<int>::make_static(std::move(static_rvalue), 7);
  if (fourth.category != 2 || fourth.value != 47 || static_rvalue.value != -1) {
    return 4;
  }

  char storage[sizeof(Target)];
  Target* placed = reinterpret_cast<Target*>(storage);
  Source placed_lvalue(50);
  Maker<int>::construct(placed, placed_lvalue, 8);
  if (placed->category != 1 || placed->value != 58 || placed_lvalue.value != 50) {
    return 5;
  }
  placed->~Target();

  Source placed_rvalue(60);
  Maker<int>::construct(placed, std::move(placed_rvalue), 9);
  if (placed->category != 2 || placed->value != 69 || placed_rvalue.value != -1) {
    return 6;
  }
  placed->~Target();

  Source traits_lvalue(70);
  TraitsLike<AllocLike<int> >::construct(placed, traits_lvalue, 10);
  if (placed->category != 1 || placed->value != 80 || traits_lvalue.value != 70) {
    return 7;
  }
  placed->~Target();

  Source traits_rvalue(80);
  TraitsLike<AllocLike<int> >::construct(placed, std::move(traits_rvalue), 11);
  if (placed->category != 2 || placed->value != 91 || traits_rvalue.value != -1) {
    return 8;
  }
  placed->~Target();

  int int_storage = 0;
  int int_value = 13;
  TraitsLike<AllocLike<int> >::construct(&int_storage, int_value);
  if (int_storage != 13) {
    return 9;
  }

  int member_storage = 0;
  int member_value = 17;
  maker.construct_member(&member_storage, member_value);
  if (member_storage != 17) {
    return 10;
  }

  AllocLike<int> alloc_like;
  int traits_alloc_storage = 0;
  int traits_alloc_value = 19;
  TraitsLike<AllocLike<int> >::construct_with_alloc(
      alloc_like, &traits_alloc_storage, traits_alloc_value);
  if (traits_alloc_storage != 19) {
    return 11;
  }

  return 0;
}
