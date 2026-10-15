// Inside a member function template, a call to another member template whose
// explicit argument is a constexpr local computed from the template parameter
// (`WriteRawLittleEndian<element_size>`, protobuf's WriteFixedPacked) selects
// the specialization for that value.  The call was left as an access to the
// uninstantiated member and jumped through the object's first field, and an
// `auto` local was passed as an unknown value.
#include <cstddef>

struct Rep {
  typedef unsigned value_type;
  int size() const { return 3; }
};

struct Stream {
  template <typename T>
  int WriteFixedPacked(const T& r) {
    constexpr auto element_size = sizeof(typename T::value_type);
    auto size = r.size() * element_size;
    return WriteRawLittleEndian<element_size>(static_cast<int>(size));
  }
  template <typename T>
  int Typed(T n) {
    constexpr int es = sizeof(T);
    return WriteRawLittleEndian<es>(n);
  }
  template <typename T>
  int Primary(T n) {
    constexpr size_t width = sizeof(T) * 2;
    return Scale<width>(n);
  }
  template <int S>
  int WriteRawLittleEndian(int size);
  template <size_t W>
  int Scale(int n) {
    return n * static_cast<int>(W) + base;
  }
  int base = 7;
};

template <>
inline int Stream::WriteRawLittleEndian<1>(int size) {
  return size + base;
}
template <>
inline int Stream::WriteRawLittleEndian<4>(int size) {
  return size * 100 + base;
}

int main() {
  Stream s;
  if (s.WriteFixedPacked(Rep{}) != 1207) {
    return 1;
  }
  if (s.Typed(3) != 307 || s.Typed('a') != 97 + 7) {
    return 2;
  }
  return s.Primary(2) == 2 * 8 + 7 ? 0 : 3;
}
