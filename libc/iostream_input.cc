#include <__iostream_input>

namespace std {

__fd_istreambuf::__fd_istreambuf(int fd) : __fd_(fd) {
  setg(__buffer_ + 1, __buffer_ + 1, __buffer_ + 1);
}

int __fd_istreambuf::underflow() {
  bool have_putback = gptr() != nullptr && eback() != nullptr &&
                      gptr() > eback();
  if (have_putback) {
    __buffer_[0] = gptr()[-1];
  }
  ssize_t count = read(__fd_, __buffer_ + 1, __buffer_size_);
  if (count <= 0) {
    return traits_type::eof();
  }
  char* begin = have_putback ? __buffer_ : __buffer_ + 1;
  setg(begin, __buffer_ + 1, __buffer_ + 1 + count);
  return traits_type::to_int_type(*gptr());
}

int __fd_istreambuf::pbackfail(int c) {
  if (traits_type::eq_int_type(c, traits_type::eof()) ||
      gptr() == nullptr || eback() == nullptr || gptr() <= eback()) {
    return traits_type::eof();
  }
  gbump(-1);
  *gptr() = traits_type::to_char_type(c);
  return c;
}

}  // namespace std
