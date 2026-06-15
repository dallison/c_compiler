// RUN: -std=c++17
enum class Color {
  red = 1,
  green = 4,
};

int main(void) {
  int value = 7;
  const int* const_ptr = &value;
  int* mutable_ptr = const_cast<int*>(const_ptr);
  void* void_ptr = reinterpret_cast<void*>(mutable_ptr);
  int* round_trip = reinterpret_cast<int*>(void_ptr);
  int enum_value = static_cast<int>(Color::green);
  Color color = static_cast<Color>(1);
  return enum_value + sizeof(round_trip) + sizeof(color);
}
