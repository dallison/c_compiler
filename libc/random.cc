#include <random>

#include <__exception_support>
#include <stdexcept>
#include <syscall.h>

namespace std {

random_device::random_device() : available_(true) {}

random_device::random_device(const string& token) : available_(true) {
  if (!token.empty() && token != "default" && token != "urandom" &&
      token != "/dev/urandom") {
    __DAVECC_THROW(runtime_error("random_device: unsupported token"));
  }
}

random_device::~random_device() {}

random_device::result_type random_device::operator()() {
  result_type value = 0;
  if (!available_ ||
      syscall(SYS_RANDOM_BYTES, &value, sizeof(value)) != 0) {
    available_ = false;
    __DAVECC_THROW(runtime_error("random_device: entropy source unavailable"));
  }
  return value;
}

double random_device::entropy() const noexcept {
  return available_ ? static_cast<double>(numeric_limits<result_type>::digits)
                    : 0.0;
}

}  // namespace std
