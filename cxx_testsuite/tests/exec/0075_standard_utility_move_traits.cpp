// RUN: -std=c++20
#include <memory>
#include <type_traits>
#include <utility>

struct ForwardedBox {
  int kind;

  ForwardedBox(int& value) : kind(value) {
  }

  ForwardedBox(int&& value) : kind(value + 10) {
  }
};

struct MoveOnlyBox {
  int value;

  explicit MoveOnlyBox(int initial) : value(initial) {
  }

  MoveOnlyBox(const MoveOnlyBox& other) = delete;

  MoveOnlyBox(MoveOnlyBox&& other) : value(other.value) {
    other.value = 0;
  }

  MoveOnlyBox& operator=(const MoveOnlyBox& other) = delete;

  MoveOnlyBox& operator=(MoveOnlyBox&& other) {
    value = other.value;
    other.value = 0;
    return *this;
  }
};

template <class T>
ForwardedBox forward_one(T&& value) {
  return ForwardedBox(std::forward<T>(value));
}

template <class... Args>
ForwardedBox forward_pack(Args&&... args) {
  return ForwardedBox(std::forward<Args>(args)...);
}

MoveOnlyBox return_implicit_move_from_local(void) {
  MoveOnlyBox local(17);
  return local;
}

MoveOnlyBox return_implicit_move_from_parameter(MoveOnlyBox value) {
  return value;
}

struct CopyArgument {
  int value;

  explicit CopyArgument(int initial) : value(initial) {
  }

  CopyArgument(const CopyArgument& other) : value(other.value) {
  }
};

int reference_after_copy_argument(CopyArgument copied, int&& value) {
  return copied.value == 23 ? value : -1;
}

int main(void) {
  static_assert(std::is_same<std::remove_reference_t<int&>, int>::value,
                "remove_reference_t<int&>");
  static_assert(std::is_lvalue_reference<int&>::value,
                "is_lvalue_reference<int&>");
  static_assert(std::is_rvalue_reference<int&&>::value,
                "is_rvalue_reference<int&&>");

  int value = 3;
  ForwardedBox lvalue_box = forward_one(value);
  ForwardedBox rvalue_box = forward_one(4);
  ForwardedBox packed_lvalue = forward_pack(value);
  ForwardedBox packed_rvalue = forward_pack(5);
  if (lvalue_box.kind != 3) {
    return 1;
  }
  if (rvalue_box.kind != 14) {
    return 8;
  }
  if (packed_lvalue.kind != 3) {
    return 9;
  }
  if (packed_rvalue.kind != 15) {
    return 10;
  }

  int old = std::exchange(value, 9);
  if (old != 3 || value != 9) {
    return 2;
  }
  std::swap(old, value);
  if (old != 9 || value != 3) {
    return 3;
  }

  std::unique_ptr<ForwardedBox> made_lvalue =
      std::make_unique<ForwardedBox>(value);
  std::unique_ptr<ForwardedBox> made_rvalue =
      std::make_unique<ForwardedBox>(6);
  if (made_lvalue.get() == 0 || made_rvalue.get() == 0) {
    return 11;
  }
  if (made_lvalue->kind != 3) {
    return 4;
  }
  if (made_rvalue->kind != 16) {
    return 12;
  }

  MoveOnlyBox local = return_implicit_move_from_local();
  MoveOnlyBox parameter =
      return_implicit_move_from_parameter(static_cast<MoveOnlyBox&&>(local));
  if (local.value != 0 || parameter.value != 17) {
    return 5;
  }

  MoveOnlyBox moved = std::move(parameter);
  if (parameter.value != 0 || moved.value != 17) {
    return 6;
  }

  MoveOnlyBox maybe_noexcept = std::move_if_noexcept(moved);
  if (moved.value != 0 || maybe_noexcept.value != 17) {
    return 7;
  }

  CopyArgument copied(23);
  if (reference_after_copy_argument(copied, 29) != 29) {
    return 13;
  }

  return 0;
}
