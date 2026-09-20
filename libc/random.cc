#include <random>

#include <cstddef>
#include <__exception_support>
#include <stdexcept>
#include <syscall.h>

#if defined(__DAVECC_NATIVE_DARWIN__)
extern "C" int getentropy(void* buffer, size_t length);
#endif

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
#if defined(__DAVECC_NATIVE_DARWIN__)
  bool failed = getentropy(&value, sizeof(value)) != 0;
#elif defined(__DAVECC_NATIVE_LINUX__)
  long result = syscall(SYS_getrandom, &value, sizeof(value), 0);
  bool failed = result != static_cast<long>(sizeof(value));
#else
  bool failed = syscall(SYS_RANDOM_BYTES, &value, sizeof(value)) != 0;
#endif
  if (!available_ ||
      failed) {
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
