// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <functional>

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

struct Qualified {
  int value;
  int mutate(void) { return ++value; }
  int lvalue_only(void) & noexcept { return value; }
  int rvalue_only(void) && noexcept { return value + 1; }
};

struct Multiplier {
  int factor;
  int operator()(int value) const noexcept { return factor * value; }
};

static int answer(void) {
  return 42;
}

using FnPM = int (Base::*)(void) const;
using DataPM = int Base::*;
using MutatingPM = int (Qualified::*)(void);
using LvaluePM = int (Qualified::*)(void) & noexcept;
using RvaluePM = int (Qualified::*)(void) && noexcept;

static_assert(std::is_invocable<FnPM, Base&>::value);
static_assert(std::is_invocable<FnPM, Derived&>::value);
static_assert(std::is_invocable<FnPM, Base*>::value);
static_assert(std::is_invocable<FnPM, std::reference_wrapper<Base>>::value);
static_assert(std::is_invocable<FnPM, PointerLike>::value);
static_assert(std::is_invocable<DataPM, PointerLike>::value);
static_assert(!std::is_invocable<MutatingPM, const Qualified&>::value);
static_assert(std::is_invocable<LvaluePM, Qualified&>::value);
static_assert(!std::is_invocable<LvaluePM, Qualified&&>::value);
static_assert(std::is_invocable<RvaluePM, Qualified&&>::value);
static_assert(!std::is_invocable<RvaluePM, Qualified&>::value);
static_assert(std::is_nothrow_invocable<LvaluePM, Qualified&>::value);
static_assert(std::is_invocable<Multiplier, int>::value);
static_assert(std::is_nothrow_invocable<Multiplier, int>::value);
static_assert(
    std::is_same<std::invoke_result_t<DataPM, const Base&>,
                 const int&>::value);

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
  if (std::invoke(fn, pointer_like) != 3) return 7;
  if (std::invoke(data, pointer_like) != 1) return 14;

  FnPMHolder fn_holder = return_fn_pm();
  if (std::invoke(fn_holder.fn, base) != 3) return 8;
  if (pass_fn_pm(fn_holder.fn, derived) != 9) return 9;

  DataPMHolder data_holder = return_data_pm();
  if (std::invoke(data_holder.data, base) != 1) return 10;
  if (pass_data_pm(data_holder.data, derived) != 4) return 11;
  if (pass_fn_pm_after_one(3, fn_holder.fn, derived) != 12) return 12;
  if (pass_fn_pm_split(1, 2, 3, fn_holder.fn, derived) != 15) return 13;

  Qualified qualified{20};
  LvaluePM lvalue_pm = &Qualified::lvalue_only;
  RvaluePM rvalue_pm = &Qualified::rvalue_only;
  if ((qualified.*lvalue_pm)() != 20) return 21;
  if ((Qualified{30}.*rvalue_pm)() != 31) return 22;
  if (std::invoke(&Qualified::mutate, qualified) != 21 ||
      qualified.value != 21)
    return 15;
  if (std::invoke(lvalue_pm, qualified) != 21) return 16;
  if (std::invoke(rvalue_pm, Qualified{30}) != 31) return 17;
  std::invoke(data, base) = 7;
  if (base.a != 7) return 18;
  if (std::invoke(Multiplier{3}, 4) != 12) return 19;
  if (std::invoke(&answer) != 42) return 20;

  return 0;
}
