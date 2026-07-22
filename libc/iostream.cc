#include <__iostream_output>

namespace std {

__fd_ostreambuf::__fd_ostreambuf(int fd) : __fd_(fd) {
  setp(__buffer_, __buffer_ + __buffer_size_);
}

__fd_ostreambuf::~__fd_ostreambuf() {
  sync();
}

int __fd_ostreambuf::overflow(int c) {
  if (sync() != 0) {
    return traits_type::eof();
  }
  if (traits_type::eq_int_type(c, traits_type::eof())) {
    return traits_type::not_eof(c);
  }
  *pptr() = traits_type::to_char_type(c);
  pbump(1);
  return c;
}

int __fd_ostreambuf::sync() {
  char* current = pbase();
  char* end = pptr();
  while (current < end) {
    ssize_t written =
        write(__fd_, current, static_cast<size_t>(end - current));
    if (written <= 0) {
      int remaining = static_cast<int>(end - current);
      for (int i = 0; i < remaining; i++) {
        __buffer_[i] = current[i];
      }
      setp(__buffer_, __buffer_ + __buffer_size_);
      pbump(remaining);
      return -1;
    }
    current += written;
  }
  setp(__buffer_, __buffer_ + __buffer_size_);
  return 0;
}

}  // namespace std
