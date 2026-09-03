// RUN: -std=c++17
// EXPECT_EXIT: 0

struct object {
  int value;
  explicit object(int initial = 42) : value(initial) {}
};

template <class T>
T* make_object() {
  return new T;
}

int main() {
  object* value = make_object<object>();
  int result = value->value == 42 ? 0 : 1;
  delete value;
  return result;
}
