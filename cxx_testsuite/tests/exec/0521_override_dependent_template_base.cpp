// RUN: -std=c++20
// EXPECT_EXIT: 0

enum result { ok, error };

template <class InternT, class ExternT, class StateT>
struct codecvt {
  typedef InternT intern_type;
  typedef ExternT extern_type;
  typedef StateT state_type;
  virtual result do_out(state_type&, const intern_type* from,
                         const intern_type*, const intern_type*& from_next,
                         extern_type* to, extern_type*,
                         extern_type*& to_next) const {
    from_next = from;
    to_next = to;
    return error;
  }
  virtual result do_unshift(state_type&, extern_type* to, extern_type*,
                             extern_type*& to_next) const {
    to_next = to;
    return error;
  }
};

template <class Elem>
struct codecvt_utf8 : codecvt<Elem, char, int> {
  result do_out(int&, const Elem* from, const Elem*, const Elem*& from_next,
                char* to, char*, char*& to_next) const override {
    from_next = from;
    to_next = to;
    return ok;
  }
  result do_unshift(int&, char* to, char*, char*& to_next) const override {
    to_next = to;
    return ok;
  }
};

int main() {
  codecvt_utf8<char> derived;
  codecvt<char, char, int>& base = derived;
  int state = 0;
  const char from[] = {'A'};
  const char* from_next = from;
  char to[4];
  char* to_next = to;
  if (base.do_out(state, from, from + 1, from_next, to, to + 4, to_next) !=
      ok) {
    return 1;
  }
  char unused[1];
  char* unused_next = unused;
  if (base.do_unshift(state, unused, unused + 1, unused_next) != ok) {
    return 2;
  }
  return 0;
}
