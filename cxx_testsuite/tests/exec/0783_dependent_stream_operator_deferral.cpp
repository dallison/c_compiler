#include <sstream>

template <class T>
struct Guard {
  static const T& get(const T& value) { return value; }
};

template <class T>
void write(std::ostream& stream, const T& value) {
  stream << Guard<T>::get(value);
}

int main() {
  std::ostringstream stream;
  struct Value {};
  Value value;
  const Value* pointer = &value;
  write(stream, pointer);
  return stream.str().empty();
}
