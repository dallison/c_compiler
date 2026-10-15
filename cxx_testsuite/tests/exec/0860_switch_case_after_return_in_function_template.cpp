// A case or default label after a statement that returns is reachable from
// its switch, so instantiating a function template keeps it.  abseil's
// FlagOps<T> has `case kCopy: ...; return nullptr;` ahead of `case
// kFastTypeId:`; dropping the later cases made every flag read abort with
// "defined as one type and declared as another".  A switch nested after an
// `if constexpr` return (protobuf's VerifyFieldType<void>) is still discarded:
// its labels are only reachable from that switch.
#include <type_traits>

template <typename T>
int SizeOrZero(int op) {
  if constexpr (std::is_void_v<T>) {
    return 0;
  }
  switch (op) {
    case 0:
      return static_cast<int>(sizeof(T));
    default:
      return -1;
  }
}

template <typename T>
int Classify(int op, T value) {
  switch (op) {
    case 0:
      (void)value;
      return 10;
    case 1:
      value = value + 1;
      return static_cast<int>(value);
    default:
      return -1;
  }
}

template <typename T>
int Nested(int op) {
  switch (op) {
    case 0: {
      T t{};
      (void)t;
      return 1;
    }
    case 1:
      return 2;
  }
  return 3;
}

int main() {
  if (Classify(0, 5) != 10) return 1;
  if (Classify(1, 5) != 6) return 2;
  if (Classify(7, 5L) != -1) return 3;
  if (Nested<int>(1) != 2) return 4;
  if (Nested<double>(9) != 3) return 5;
  if (SizeOrZero<void>(0) != 0) return 6;
  if (SizeOrZero<int>(0) != 4 || SizeOrZero<int>(3) != -1) return 7;
  return 0;
}
