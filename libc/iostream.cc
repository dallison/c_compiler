#include <iostream>

namespace std {

__stdio_istream::__stdio_istream(FILE* file)
    : basic_istream<char, char_traits<char>>() {
  __buf_.__attach(file, ios_base::in);
  this->init(&__buf_);
}

__stdio_ostream::__stdio_ostream(FILE* file)
    : basic_ostream<char, char_traits<char>>() {
  __buf_.__attach(file, ios_base::out);
  this->init(&__buf_);
}

__iostream_initializer::__iostream_initializer() {
  cin.tie(&cout);
  cerr.tie(&cout);
  cerr.setf(ios_base::unitbuf);
}

__stdio_ostream cout(stdout);
__stdio_istream cin(stdin);
__stdio_ostream cerr(stderr);
__stdio_ostream clog(stderr);
__iostream_initializer __iostream_init;

}  // namespace std
