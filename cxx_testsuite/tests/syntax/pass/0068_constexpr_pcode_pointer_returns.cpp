// RUN: -std=c++20

struct PCodePointerPair {
  int left;
  int right;
};

constexpr PCodePointerPair pcode_global_pair = {10, 32};

constexpr int* pcode_second_pointer(int* left, int* right) {
  (void)left;
  return right;
}

constexpr int* pcode_select_pointer(int* left, int* right, bool choose_right) {
  return choose_right ? right : left;
}

constexpr int* pcode_right_member_pointer(PCodePointerPair* pair) {
  return &pair->right;
}

constexpr int* pcode_select_member_pointer(PCodePointerPair* pair, bool right) {
  return right ? &pair->right : &pair->left;
}

constexpr int* pcode_heap_pointer(int value) {
  int* result = new int(value);
  return result;
}

constexpr int pcode_pointer_return_values(void) {
  int values[3] = {10, 20, 12};
  int* selected = pcode_second_pointer(&values[0], &values[1]);
  PCodePointerPair pair = {5, 7};
  int* member = pcode_right_member_pointer(&pair);
  int* heap = pcode_heap_pointer(3);
  int result = *selected + values[2] + *member + *heap;
  delete heap;
  return result;
}

constexpr int pcode_select_pointer_value(void) {
  int values[2] = {10, 32};
  int* selected = pcode_second_pointer(&values[0], &values[1]);
  return *selected;
}

constexpr int pcode_member_pointer_value(void) {
  PCodePointerPair pair = {10, 32};
  int* member = pcode_right_member_pointer(&pair);
  return *member;
}

constexpr int pcode_conditional_pointer_value(void) {
  int values[2] = {10, 32};
  int* selected = pcode_select_pointer(&values[0], &values[1], true);
  return *selected;
}

constexpr int pcode_conditional_member_pointer_value(void) {
  PCodePointerPair pair = {10, 32};
  int* member = pcode_select_member_pointer(&pair, true);
  return *member;
}

constexpr int pcode_heap_pointer_value(void) {
  int* heap = pcode_heap_pointer(42);
  int result = *heap;
  delete heap;
  return result;
}

constexpr const int* pcode_global_right_pointer(const PCodePointerPair* pair) {
  return &pair->right;
}

static_assert(*pcode_global_right_pointer(&pcode_global_pair) == 32,
              "pcode constexpr top-level pointer return");
static_assert(pcode_select_pointer_value() == 32,
              "pcode constexpr selected pointer return");
static_assert(pcode_member_pointer_value() == 32,
              "pcode constexpr member pointer return");
static_assert(pcode_conditional_pointer_value() == 32,
              "pcode constexpr conditional pointer return");
static_assert(pcode_conditional_member_pointer_value() == 32,
              "pcode constexpr conditional member pointer return");
static_assert(pcode_heap_pointer_value() == 42,
              "pcode constexpr heap pointer return");
static_assert(pcode_pointer_return_values() == 42,
              "pcode constexpr pointer returns");
