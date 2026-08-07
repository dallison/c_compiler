// RUN: -std=c++20

template <class CharT, class Traits = int>
struct basic_streambuf {
  typedef CharT char_type;
  virtual int sync() { return 0; }
  virtual long xsputn(const char_type*, long) { return 0; }
};

using streambuf_alias = basic_streambuf<char>;

struct DerivedStreambuf : streambuf_alias {
  int sync() override { return 1; }
  long xsputn(const char*, long) override { return 1; }
};

struct Base {
  virtual int sync() { return 0; }
};

using BaseAlias = Base;

struct DerivedBase : BaseAlias {
  int sync() override { return 2; }
};

int main() {
  DerivedStreambuf streambuf;
  DerivedBase base;
  return streambuf.sync() + base.sync() - 3;
}
