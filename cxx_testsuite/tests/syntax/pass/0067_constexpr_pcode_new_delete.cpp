// RUN: -std=c++20

struct PCodeNewBox {
  int value;

  constexpr PCodeNewBox(int v) : value(v) {}
  constexpr int read(void) const { return value; }
};

constexpr int pcode_scalar_new_delete(void) {
  int* value = new int(42);
  int result = *value;
  delete value;
  return result;
}

constexpr int pcode_array_new_delete(void) {
  int* values = new int[4];
  values[0] = 10;
  values[1] = 11;
  values[2] = 9;
  values[3] = 12;
  int result = values[0] + values[1] + values[2] + values[3];
  delete[] values;
  return result;
}

constexpr int pcode_object_new_delete(void) {
  PCodeNewBox* box = new PCodeNewBox(42);
  int result = box->read();
  delete box;
  return result;
}

static_assert(pcode_scalar_new_delete() == 42,
              "pcode constexpr scalar new/delete");
static_assert(pcode_array_new_delete() == 42,
              "pcode constexpr array new/delete");
static_assert(pcode_object_new_delete() == 42,
              "pcode constexpr object new/delete");
