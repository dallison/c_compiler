#include <__iostream_output>

namespace std {

__stdio_ostreambuf::__stdio_ostreambuf(FILE* file) : __file_(file) {}

int __stdio_ostreambuf::overflow(int c) {
  if (traits_type::eq_int_type(c, traits_type::eof())) {
    return traits_type::not_eof(c);
  }
  char ch = traits_type::to_char_type(c);
  return fputc(static_cast<unsigned char>(ch), __file_) < 0
             ? traits_type::eof()
             : c;
}

streamsize __stdio_ostreambuf::xsputn(const char* s, streamsize n) {
  if (n <= 0) {
    return 0;
  }
  return static_cast<streamsize>(
      fwrite(s, 1, static_cast<size_t>(n), __file_));
}

int __stdio_ostreambuf::sync() {
  return fflush(__file_) == 0 ? 0 : -1;
}

}  // namespace std
