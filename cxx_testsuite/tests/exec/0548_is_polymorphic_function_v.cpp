// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <memory>
#include <type_traits>

void free_fn();

struct Plain {
  int value;
  void method();
};

struct Poly {
  virtual void method();
};

struct Derived : Poly {};

struct Empty {};
struct EmptyFinal final {};

int main() {
  static_assert(!std::is_function_v<int>);
  static_assert(std::is_function_v<void()>);
  static_assert(std::is_function_v<int(int, char)>);
  static_assert(!std::is_function_v<void (*)()>);

  static_assert(std::is_member_function_pointer_v<void (Poly::*)()>);
  static_assert(!std::is_member_function_pointer_v<int Poly::*>);
  static_assert(std::is_member_object_pointer_v<int Plain::*>);
  static_assert(!std::is_member_object_pointer_v<void (Plain::*)()>);

  static_assert(!std::is_polymorphic<int>::value);
  static_assert(!std::is_polymorphic<Plain>::value);
  static_assert(std::is_polymorphic<Poly>::value);
  static_assert(std::is_polymorphic<Derived>::value);
  static_assert(std::is_polymorphic<const Poly>::value);
  static_assert(std::is_polymorphic_v<Poly>);
  static_assert(!std::is_polymorphic_v<Plain>);

  static_assert(std::is_empty<Empty>::value);
  static_assert(!std::is_empty<Plain>::value);
  static_assert(!std::is_empty<Poly>::value);
  static_assert(std::is_empty_v<Empty>);
  static_assert(!std::is_final<Empty>::value);
  static_assert(std::is_final<EmptyFinal>::value);
  static_assert(std::is_final_v<EmptyFinal>);

  const int const_value = 7;
  if (std::addressof(const_value) != &const_value) {
    return 1;
  }
  static_assert(std::extent<int[3]>::value == 3);
  static_assert(std::extent_v<int[3], 0> == 3);

  static_assert(__builtin_bswap16((unsigned short)0x1234) == 0x3412);
  static_assert(__builtin_bswap32(0x12345678u) == 0x78563412u);
  static_assert(__builtin_bswap64(0x0123456789ABCDEFULL) ==
                0xEFCDAB8967452301ULL);
  return 0;
}
