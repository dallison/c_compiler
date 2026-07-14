// RUN: -std=c++20
struct Base {
  int a;
  int b;
  int get(void) const { return a + b; }
  int get_noexcept(void) const noexcept { return a + b; }
};

struct Derived : Base {
  int c;
  int get(void) const { return a + b + c; }
};

struct Mid : Base {
  int m;
};

struct Left : virtual Base {
  int l;
};

struct Right : virtual Base {
  int r;
};

struct Diamond : Left, Right {
  int d;
  Diamond(void) : Base(), Left(), Right(), d(0) {
    a = 1;
    b = 2;
    l = 3;
    r = 4;
    d = 5;
  }
};

struct Virt {
  virtual int f(void) const { return 1; }
};

struct VirtDerived : Virt {
  int x;
  int f(void) const override { return x + 10; }
};

struct MultiLeft {
  int left;
};

struct MultiRight {
  int right;
  int read_right(void) const { return right; }
};

struct Multi : MultiLeft, MultiRight {
  int own;
};

struct DataPM {
  int Base::*pm;
};

static int pass_data_pm(int Base::*pm, Base& obj) {
  return obj.*pm;
}

static int pass_data_pm_noinline(DataPM holder, Base& obj) {
  return pass_data_pm(holder.pm, obj);
}

static int pass_fn_pm(Base& obj) {
  int (Base::*pf)(void) const = &Base::get;
  return (obj.*pf)();
}

static int pass_fn_pm_noinline(Base& obj) {
  return pass_fn_pm(obj);
}

static DataPM return_data_pm(void) {
  DataPM holder{&Base::a};
  return holder;
}

struct PMHolder {
  int Base::*data_pm;
};

static int use_holder(PMHolder holder, Base& obj) {
  return obj.*holder.data_pm + pass_fn_pm(obj);
}

static int choose_pm(bool pick_a, Base& obj) {
  int Base::*pa = &Base::a;
  int Base::*pb = &Base::b;
  if (pick_a) {
    return obj.*pa;
  }
  return obj.*pb;
}

static bool pm_null(DataPM holder) {
  return holder.pm == nullptr;
}

static bool pm_eq(DataPM lhs, DataPM rhs) {
  return lhs.pm == rhs.pm;
}

int Base::*gpa = &Base::a;
constexpr int Base::*constexpr_pb = &Base::b;
constexpr int (Base::*constexpr_get)(void) const = &Base::get;
static_assert(constexpr_pb != nullptr);
static_assert(constexpr_get != nullptr);

static int receiver_evaluations;

static Base* evaluated_receiver(Base* object) {
  ++receiver_evaluations;
  return object;
}

int main(void) {
  Base base;
  base.a = 1;
  base.b = 2;
  Derived derived;
  derived.a = 4;
  derived.b = 5;
  derived.c = 6;

  int Base::*pa = &Base::a;

  if (base.*pa != 1) return 1;
  if (base.*constexpr_pb != 2) return 18;
  if ((base.*constexpr_get)() != 3) return 19;
  if (evaluated_receiver(&base)->*pa != 1 || receiver_evaluations != 1)
    return 21;
  if ((evaluated_receiver(&base)->*constexpr_get)() != 3 ||
      receiver_evaluations != 2)
    return 22;
  if (derived.*gpa != 4) return 4;
  if (pass_data_pm(pa, base) != 1) return 2;
  if (pass_data_pm_noinline(DataPM{pa}, derived) != 4) return 3;
  if (pass_fn_pm_noinline(base) != 3) return 5;
  if (use_holder(PMHolder{pa}, derived) != 13) return 6;
  if (choose_pm(true, base) != 1 || choose_pm(false, base) != 2) return 7;

  DataPM returned = return_data_pm();
  if (returned.pm != pa || pm_null(returned)) return 8;
  if (!pm_null(DataPM{nullptr})) return 9;
  if (!pm_eq(returned, DataPM{pa})) return 27;

  int Derived::*derived_pa = pa;
  if (derived.*derived_pa != 4) return 10;
  int Base::*round_trip = static_cast<int Base::*>(derived_pa);
  if (base.*round_trip != 1) return 11;
  const int Base::*const_pa = pa;
  if (base.*const_pa != 1) return 23;
  int Derived::*null_derived = nullptr;
  if (null_derived != nullptr) return 26;
  int Base::*null_base = static_cast<int Base::*>(null_derived);
  if (null_base != nullptr) return 24;
  int (Base::*noexcept_fn)(void) const noexcept = &Base::get_noexcept;
  int (Base::*throwing_fn)(void) const = noexcept_fn;
  if ((base.*throwing_fn)() != 3) return 25;

  VirtDerived virt;
  virt.x = 7;
  int (Virt::*virtual_fn)(void) const = &Virt::f;
  if ((virt.*virtual_fn)() != 17) return 12;

  Multi multi;
  multi.left = 20;
  multi.right = 21;
  multi.own = 22;
  int MultiRight::*right_data = &MultiRight::right;
  int (MultiRight::*right_fn)(void) const = &MultiRight::read_right;
  if (multi.*right_data != 21 || (multi.*right_fn)() != 21) return 13;
  int Multi::*converted_data = right_data;
  int (Multi::*converted_fn)(void) const = right_fn;
  int converted_data_value = multi.*converted_data;
  if (converted_data_value != 21) return 14;
  if ((multi.*converted_fn)() != 21) return 17;

  Diamond diamond;
  if (diamond.*pa != 1) return 15;
  int (Base::*base_fn)(void) const = &Base::get;
  if ((diamond.*base_fn)() != 3) return 16;
  return 0;
}
