#include <__iostream_input>

namespace std {

__stdio_istreambuf::__stdio_istreambuf(FILE* file) : __file_(file) {}

int __stdio_istreambuf::underflow() {
  int c = fgetc(__file_);
  if (c < 0 || ungetc(c, __file_) < 0) {
    return traits_type::eof();
  }
  return traits_type::to_int_type(static_cast<char>(c));
}

int __stdio_istreambuf::uflow() {
  int c = fgetc(__file_);
  return c < 0 ? traits_type::eof()
               : traits_type::to_int_type(static_cast<char>(c));
}

int __stdio_istreambuf::pbackfail(int c) {
  if (traits_type::eq_int_type(c, traits_type::eof()) ||
      ungetc(static_cast<unsigned char>(traits_type::to_char_type(c)),
             __file_) < 0) {
    return traits_type::eof();
  }
  return c;
}

streamsize __stdio_istreambuf::xsgetn(char* s, streamsize n) {
  if (n <= 0) {
    return 0;
  }
  return static_cast<streamsize>(
      fread(s, 1, static_cast<size_t>(n), __file_));
}

}  // namespace std
