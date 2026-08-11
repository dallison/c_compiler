// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>
#include <stdexcept>

int main() {
  std::random_device device;
  static_assert(std::random_device::min() == 0);
  static_assert(std::random_device::max() > 0);

  if (device.entropy() <= 0.0) {
    return 1;
  }

  const auto first = device();
  bool changed = false;
  for (int i = 0; i < 8; ++i) {
    if (device() != first) {
      changed = true;
    }
  }
  if (!changed) {
    return 2;
  }

  std::random_device named("urandom");
  (void)named();

#ifdef __cpp_exceptions
  bool rejected = false;
  try {
    std::random_device invalid("unsupported-random-device");
  } catch (const std::runtime_error&) {
    rejected = true;
  }
  return rejected ? 0 : 3;
#else
  return 0;
#endif
}
