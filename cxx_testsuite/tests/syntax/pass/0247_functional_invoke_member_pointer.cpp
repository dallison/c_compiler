// RUN: -std=c++20
#include <functional>
#include <type_traits>

struct Base {
  int a;
  int b;
  int sum(void) const { return a + b; }
};

struct Derived : Base {
  int c;
  int sum(void) const { return a + b + c; }
};

struct PointerLike {
  Base* ptr;
  Base& operator*(void) const { return *ptr; }
};

struct FnPMHolder {
  int (Base::*fn)(void) const;
};

struct DataPMHolder {
  int Base::*data;
};

static int pass_fn_pm(int (Base::*fn)(void) const, Base& obj) {
  return std::invoke(fn, obj);
}

static int pass_data_pm(int Base::*data, Base& obj) {
  return std::invoke(data, obj);
}

static int pass_fn_pm_after_one(int prefix, int (Base::*fn)(void) const,
                                Base& obj) {
  return prefix + std::invoke(fn, obj);
}

static int pass_fn_pm_split(int a, int b, int c,
                            int (Base::*fn)(void) const, Base& obj) {
  return a + b + c + std::invoke(fn, obj);
}

static FnPMHolder return_fn_pm(void) {
  FnPMHolder holder{&Base::sum};
  return holder;
}

static DataPMHolder return_data_pm(void) {
  DataPMHolder holder{&Base::a};
  return holder;
}

static_assert(std::is_invocable<int (Base::*)(void) const, Base>::value);
static_assert(std::is_invocable<int (Base::*)(void) const, Derived>::value);
static_assert(std::is_invocable<int (Base::*)(void) const, Base*>::value);
static_assert(std::is_invocable<int Base::*, Base&>::value);
static_assert(std::is_invocable<int Base::*, Base*>::value);
static_assert(
    std::is_same<std::invoke_result_t<int (Base::*)(void) const, Base>, int>::
        value);
static_assert(!std::is_invocable<int (Base::*)(void) const, Base, int>::value);
static_assert(!std::is_invocable<int Base::*, int>::value);

int main(void) {
  Base base{1, 2};
  Derived derived{4, 5, 6};

  int (Base::*fn)(void) const = &Base::sum;
  int Base::*data = &Base::a;

  if (std::invoke(fn, base) != 3) return 1;
  if (std::invoke(fn, derived) != 9) return 2;
  if (std::invoke(fn, &base) != 3) return 3;
  if (std::invoke(fn, std::ref(base)) != 3) return 4;
  if (std::invoke(data, base) != 1) return 5;
  if (std::invoke(data, &derived) != 4) return 6;

  PointerLike pointer_like{&base};
  if (std::invoke(fn, *pointer_like) != 3) return 7;

  FnPMHolder fn_holder = return_fn_pm();
  if (std::invoke(fn_holder.fn, base) != 3) return 8;
  if (pass_fn_pm(fn_holder.fn, derived) != 9) return 9;

  DataPMHolder data_holder = return_data_pm();
  if (std::invoke(data_holder.data, base) != 1) return 10;
  if (pass_data_pm(data_holder.data, derived) != 4) return 11;
  if (pass_fn_pm_after_one(3, fn_holder.fn, derived) != 12) return 12;
  if (pass_fn_pm_split(1, 2, 3, fn_holder.fn, derived) != 15) return 13;

  return 0;
}
