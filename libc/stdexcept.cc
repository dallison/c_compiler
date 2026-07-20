#include <stdexcept>

char* std::__detail::__what_holder::__dup(const char* s) noexcept {
  if (s == nullptr) {
    s = "";
  }
  size_t n = 0;
  while (s[n] != '\0') {
    n++;
  }
  char* p = static_cast<char*>(::operator new(n + 1));
  for (size_t i = 0; i <= n; i++) {
    p[i] = s[i];
  }
  return p;
}

std::__detail::__what_holder::__what_holder(const char* s)
    : __msg_(__dup(s)) {}

std::__detail::__what_holder::__what_holder(
    const std::__detail::__what_holder& other) noexcept
    : exception(other), __msg_(__dup(other.__msg_)) {}

std::__detail::__what_holder& std::__detail::__what_holder::operator=(
    const std::__detail::__what_holder& other) noexcept {
  if (this != &other) {
    char* fresh = __dup(other.__msg_);
    ::operator delete(static_cast<void*>(__msg_));
    __msg_ = fresh;
  }
  return *this;
}

std::__detail::__what_holder::~__what_holder() {
  ::operator delete(static_cast<void*>(__msg_));
}

const char* std::__detail::__what_holder::what() const noexcept {
  return __msg_;
}

std::logic_error::logic_error(const std::string& what_arg)
    : __what_holder(what_arg.c_str()) {}

std::logic_error::logic_error(const char* what_arg)
    : __what_holder(what_arg) {}

std::runtime_error::runtime_error(const std::string& what_arg)
    : __what_holder(what_arg.c_str()) {}

std::runtime_error::runtime_error(const char* what_arg)
    : __what_holder(what_arg) {}

#define __DAVECC_DEFINE_ERROR(NAME, BASE)                              \
  std::NAME::NAME(const std::string& what_arg) : BASE(what_arg) {}     \
  std::NAME::NAME(const char* what_arg) : BASE(what_arg) {}

__DAVECC_DEFINE_ERROR(domain_error, logic_error)
__DAVECC_DEFINE_ERROR(invalid_argument, logic_error)
__DAVECC_DEFINE_ERROR(length_error, logic_error)
__DAVECC_DEFINE_ERROR(out_of_range, logic_error)
__DAVECC_DEFINE_ERROR(range_error, runtime_error)
__DAVECC_DEFINE_ERROR(overflow_error, runtime_error)
__DAVECC_DEFINE_ERROR(underflow_error, runtime_error)

#undef __DAVECC_DEFINE_ERROR
