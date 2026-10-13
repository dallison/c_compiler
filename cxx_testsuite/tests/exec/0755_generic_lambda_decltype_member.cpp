// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

struct Descriptor {
  using OptionsType = int;
  const int* options_;
};

struct OtherDescriptor {
  using OptionsType = char;
  const char* options_;
};

int main() {
  int first = 1;
  char second = 2;
  const Descriptor descriptor{&first};
  const OtherDescriptor other{&second};
  auto inspect = [](const auto& value) {
    using Options =
        typename std::remove_const<typename std::remove_pointer<
            decltype(value.options_)>::type>::type;
    using Value =
        typename std::remove_const<typename std::remove_reference<
            decltype(value)>::type>::type;
    static_assert(std::is_same<Options, typename Value::OptionsType>::value);
    auto options = const_cast<Options*>(value.options_);
    return sizeof(*options);
  };
  if (inspect(descriptor) != sizeof(int)) {
    return 1;
  }
  return inspect(other) == sizeof(char) ? 0 : 2;
}
