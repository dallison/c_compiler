struct Value {};

int select(const Value&) { return 1; }

template <class T>
int select(T&&) {
  return 2;
}

int main() { return select(Value{}) != 2; }
