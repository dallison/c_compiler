// RUN: -std=c++20

#include <ostream>

struct Base {
  void foo(int) {}
  void foo() {}
  int set(int x) { return x; }
};

struct Derived : Base {
  void foo() {}
  int set(int x) { return x + 1; }
  void use() {
    Base::foo(1);
    (void)Base::set(2);
  }
};

struct MoveBase {
  MoveBase& operator=(MoveBase&&) { return *this; }
};

struct MoveDerived : MoveBase {
  MoveDerived& operator=(MoveDerived&&) { return *this; }
  void assign(MoveDerived&& other) {
    MoveBase::operator=(static_cast<MoveBase&&>(other));
  }
};

struct StreamBuf {};

template <class CharT>
struct BasicIOS {
  void rdbuf(StreamBuf* buf) { (void)buf; }
  StreamBuf* rdbuf() { return nullptr; }
};

template <class CharT>
struct BasicOStream : BasicIOS<CharT> {
  BasicOStream& operator=(BasicOStream&&) { return *this; }
};

template <class CharT>
struct DerivedOS : BasicOStream<CharT> {
  DerivedOS() = default;
  StreamBuf* rdbuf() { return nullptr; }
  DerivedOS& operator=(DerivedOS&&) { return *this; }

  void call_rdbuf() {
    BasicIOS<CharT>::rdbuf(&buffer_);
  }

  void call_assign(DerivedOS&& other) {
    BasicOStream<CharT>::operator=(static_cast<BasicOStream<CharT>&&>(other));
    buffer_ = static_cast<StreamBuf&&>(other.buffer_);
    BasicIOS<CharT>::rdbuf(&buffer_);
  }

  StreamBuf buffer_;
};

void test_dependent_qualified_base_lookup() {
  DerivedOS<char> left;
  DerivedOS<char> right;
  left = static_cast<DerivedOS<char>&&>(right);
  left.call_rdbuf();
  left.call_assign(static_cast<DerivedOS<char>&&>(right));
}

template <class CharT, class Traits>
struct VirtualDerivedOS : std::basic_ostream<CharT, Traits> {
  std::basic_streambuf<CharT, Traits>* rdbuf() const { return nullptr; }

  void call_rdbuf() {
    std::basic_ios<CharT, Traits>::rdbuf(&buffer_);
  }

  void call_assign(VirtualDerivedOS&& other) {
    std::basic_ostream<CharT, Traits>::operator=(
        static_cast<std::basic_ostream<CharT, Traits>&&>(other));
    buffer_ = static_cast<VirtualStreamBuf&&>(other.buffer_);
    std::basic_ios<CharT, Traits>::rdbuf(&buffer_);
  }

  struct VirtualStreamBuf : std::basic_streambuf<CharT, Traits> {};
  VirtualStreamBuf buffer_;
};

void test_virtual_dependent_qualified_base_lookup() {
  VirtualDerivedOS<char, std::char_traits<char>> left;
  VirtualDerivedOS<char, std::char_traits<char>> right;
  left.call_rdbuf();
  left.call_assign(static_cast<VirtualDerivedOS<char, std::char_traits<char>>&&>(
      right));
}

int main() {
  Derived d;
  d.use();
  MoveDerived md;
  MoveDerived other;
  md.assign(static_cast<MoveDerived&&>(other));
  test_dependent_qualified_base_lookup();
  test_virtual_dependent_qualified_base_lookup();
  return 0;
}
