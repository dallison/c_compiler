// RUN: -std=c++20

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
