// RUN: -std=c++20
void* operator new(unsigned long size, void* ptr);

struct PCodeLifetimeInner {
  int value;

  constexpr PCodeLifetimeInner(int v) : value(v) {}
};

struct PCodeLifetimeOuter {
  PCodeLifetimeInner left;
  PCodeLifetimeInner right;

  constexpr PCodeLifetimeOuter(int l, int r) : left{l}, right{r} {}
};

struct PCodeLifetimeBox {
  int value;

  constexpr PCodeLifetimeBox(int v) : value(v) {}
  constexpr void add(int delta) { value += delta; }
  constexpr int read(void) const { return value; }
};

struct PCodeLifetimeDestructorProbe {
  int* target;
  int delta;

  constexpr PCodeLifetimeDestructorProbe(int* t, int d) : target(t), delta(d) {}
  constexpr ~PCodeLifetimeDestructorProbe() { *target += delta; }
};

struct PCodeLifetimeArrayDestructorProbe {
  int* target;
  int delta;

  constexpr PCodeLifetimeArrayDestructorProbe() : target(0), delta(0) {}
  constexpr void bind(int* t, int d) {
    target = t;
    delta = d;
  }
  constexpr ~PCodeLifetimeArrayDestructorProbe() {
    if (target != 0) {
      *target += delta;
    }
  }
};

struct PCodeLifetimeProbePair {
  PCodeLifetimeDestructorProbe first;
  PCodeLifetimeDestructorProbe second;

  constexpr PCodeLifetimeProbePair(int* target)
      : first(target, 1), second(target, 2) {}
};

struct PCodeLifetimeDefaultedCopy {
  int left;
  int right;

  constexpr PCodeLifetimeDefaultedCopy(int l, int r) : left(l), right(r) {}
  constexpr PCodeLifetimeDefaultedCopy(
      const PCodeLifetimeDefaultedCopy& other) = default;
  constexpr struct PCodeLifetimeDefaultedCopy& operator=(
      const PCodeLifetimeDefaultedCopy& other) = default;
};

struct PCodeLifetimeImplicitCopy {
  int value;

  constexpr PCodeLifetimeImplicitCopy(int v) : value(v) {}
};

constexpr int& pcode_right_ref(int& left, int& right) {
  (void)left;
  return right;
}

constexpr int& pcode_select_ref(int& left, int& right, bool choose_right) {
  return choose_right ? right : left;
}

constexpr PCodeLifetimeInner& pcode_right_inner(PCodeLifetimeOuter& outer) {
  return outer.right;
}

constexpr PCodeLifetimeInner& pcode_select_inner(PCodeLifetimeOuter& outer,
                                                 bool choose_right) {
  return choose_right ? outer.right : outer.left;
}

constexpr int pcode_reference_return_lvalue(void) {
  int left = 10;
  int right = 32;
  int& selected = pcode_right_ref(left, right);
  selected += 10;
  return left + right;
}

constexpr int pcode_conditional_reference_return_lvalue(void) {
  int left = 10;
  int right = 32;
  int& selected = pcode_select_ref(left, right, true);
  selected += 10;
  return left + right;
}

constexpr int pcode_nested_reference_member_mutation(void) {
  PCodeLifetimeOuter outer(10, 20);
  PCodeLifetimeInner& selected = pcode_right_inner(outer);
  selected.value += 12;
  return outer.left.value + outer.right.value;
}

constexpr int pcode_conditional_nested_reference_member_mutation(void) {
  PCodeLifetimeOuter outer(10, 20);
  PCodeLifetimeInner& selected = pcode_select_inner(outer, true);
  selected.value += 12;
  return outer.left.value + outer.right.value;
}

constexpr int pcode_heap_object_pointer_mutation(void) {
  PCodeLifetimeBox* box = new PCodeLifetimeBox(10);
  box->add(32);
  int result = box->read();
  delete box;
  return result;
}

constexpr int pcode_heap_array_loop_sum(void) {
  int* values = new int[6];
  int result = 0;
  for (int i = 0; i < 6; i++) {
    values[i] = i + 2;
    result += values[i];
  }
  delete[] values;
  return result;
}

constexpr int pcode_nested_heap_member_pointer(void) {
  PCodeLifetimeOuter* outer = new PCodeLifetimeOuter(5, 11);
  PCodeLifetimeInner* selected = &outer->right;
  selected->value += 26;
  int result = outer->left.value + outer->right.value;
  delete outer;
  return result;
}

constexpr int pcode_scoped_destructor_side_effect(void) {
  int result = 10;
  {
    PCodeLifetimeDestructorProbe first(&result, 7);
    PCodeLifetimeDestructorProbe second(&result, 25);
    (void)first;
    (void)second;
  }
  return result;
}

constexpr int pcode_delete_destructor_side_effect(void) {
  int result = 10;
  PCodeLifetimeDestructorProbe* probe =
      new PCodeLifetimeDestructorProbe(&result, 32);
  delete probe;
  return result;
}

constexpr int pcode_scoped_array_destructor_side_effect(void) {
  int result = 10;
  {
    PCodeLifetimeDestructorProbe probes[2] = {
        PCodeLifetimeDestructorProbe(&result, 7),
        PCodeLifetimeDestructorProbe(&result, 25)};
    (void)probes;
  }
  return result;
}

constexpr int pcode_delete_array_destructor_side_effect(void) {
  int result = 10;
  PCodeLifetimeArrayDestructorProbe* probes =
      new PCodeLifetimeArrayDestructorProbe[2];
  probes[0].bind(&result, 7);
  probes[1].bind(&result, 25);
  delete[] probes;
  return result;
}

constexpr int pcode_placement_new_object_reuse(void) {
  PCodeLifetimeBox storage(10);
  PCodeLifetimeBox* box = new ((void*)&storage) PCodeLifetimeBox(32);
  box->add(10);
  return storage.read();
}

constexpr int pcode_placement_new_scalar_reuse(void) {
  int storage = 10;
  int* value = new ((void*)&storage) int(42);
  return storage + *value;
}

constexpr int pcode_explicit_destructor_arrow_call(void) {
  int result = 10;
  PCodeLifetimeDestructorProbe storage(&result, 1);
  PCodeLifetimeDestructorProbe* probe =
      new ((void*)&storage) PCodeLifetimeDestructorProbe(&result, 32);
  probe->~PCodeLifetimeDestructorProbe();
  return result;
}

constexpr int pcode_explicit_destructor_dot_reuse(void) {
  int result = 10;
  {
    PCodeLifetimeDestructorProbe probe(&result, 1);
    probe.~PCodeLifetimeDestructorProbe();
    new ((void*)&probe) PCodeLifetimeDestructorProbe(&result, 31);
  }
  return result;
}

constexpr int pcode_placement_new_array_element_reuse(void) {
  int result = 9;
  {
    PCodeLifetimeDestructorProbe probes[2] = {
        PCodeLifetimeDestructorProbe(&result, 1),
        PCodeLifetimeDestructorProbe(&result, 2)};
    probes[1].~PCodeLifetimeDestructorProbe();
    new ((void*)&probes[1]) PCodeLifetimeDestructorProbe(&result, 30);
  }
  return result;
}

constexpr int pcode_placement_new_subobject_reuse(void) {
  PCodeLifetimeOuter outer(10, 1);
  PCodeLifetimeInner* inner =
      new ((void*)&outer.right) PCodeLifetimeInner(32);
  inner->value += 0;
  return outer.left.value + outer.right.value;
}

constexpr int pcode_implicit_member_destructor_cleanup(void) {
  int result = 39;
  {
    PCodeLifetimeProbePair pair(&result);
    (void)pair;
  }
  return result;
}

constexpr int pcode_defaulted_copy_and_assignment(void) {
  PCodeLifetimeDefaultedCopy first(10, 11);
  PCodeLifetimeDefaultedCopy second(first);
  PCodeLifetimeDefaultedCopy third(1, 2);
  third = second;
  return first.left + second.right + third.left + third.right;
}

constexpr int pcode_implicit_special_member_copy(void) {
  PCodeLifetimeImplicitCopy first(42);
  PCodeLifetimeImplicitCopy second(first);
  return second.value;
}

static_assert(pcode_reference_return_lvalue() == 52,
              "pcode constexpr reference return lvalue");
static_assert(pcode_conditional_reference_return_lvalue() == 52,
              "pcode constexpr conditional reference return lvalue");
static_assert(pcode_nested_reference_member_mutation() == 42,
              "pcode constexpr nested reference member mutation");
static_assert(pcode_conditional_nested_reference_member_mutation() == 42,
              "pcode constexpr conditional nested reference member mutation");
static_assert(pcode_heap_object_pointer_mutation() == 42,
              "pcode constexpr heap object pointer mutation");
static_assert(pcode_heap_array_loop_sum() == 27,
              "pcode constexpr heap array loop sum");
static_assert(pcode_nested_heap_member_pointer() == 42,
              "pcode constexpr nested heap member pointer");
static_assert(pcode_scoped_destructor_side_effect() == 42,
              "pcode constexpr scoped destructor side effect");
static_assert(pcode_delete_destructor_side_effect() == 42,
              "pcode constexpr delete destructor side effect");
static_assert(pcode_scoped_array_destructor_side_effect() == 42,
              "pcode constexpr scoped array destructor side effect");
static_assert(pcode_delete_array_destructor_side_effect() == 42,
              "pcode constexpr delete array destructor side effect");
#if __cplusplus > 202302L
static_assert(pcode_placement_new_object_reuse() == 42,
              "pcode constexpr placement new object reuse");
static_assert(pcode_placement_new_scalar_reuse() == 84,
              "pcode constexpr placement new scalar reuse");
static_assert(pcode_explicit_destructor_arrow_call() == 42,
              "pcode constexpr explicit destructor arrow call");
static_assert(pcode_explicit_destructor_dot_reuse() == 42,
              "pcode constexpr explicit destructor dot reuse");
static_assert(pcode_placement_new_array_element_reuse() == 42,
              "pcode constexpr placement new array element reuse");
static_assert(pcode_placement_new_subobject_reuse() == 42,
              "pcode constexpr placement new subobject reuse");
#endif
static_assert(pcode_implicit_member_destructor_cleanup() == 42,
              "pcode constexpr implicit member destructor cleanup");
static_assert(pcode_defaulted_copy_and_assignment() == 42,
              "pcode constexpr defaulted copy and assignment");
static_assert(pcode_implicit_special_member_copy() == 42,
              "pcode constexpr implicit special member copy");
