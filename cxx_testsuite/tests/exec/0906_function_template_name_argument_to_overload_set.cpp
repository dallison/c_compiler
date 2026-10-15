// A function template name passed to an overload set containing a member
// template: the template candidate cannot deduce from it ([temp.deduct.call]
// p6) and the function-pointer overload takes it ([over.over]).  The call is
// not dependent, so `msg << std::endl << "x"` keeps its `Message&` type
// (googletest's Message).
#include <ostream>
#include <sstream>
#include <string>

struct Message {
  std::stringstream ss;
  template <typename T> Message& operator<<(const T& value) {
    ss << value;
    return *this;
  }
  typedef std::ostream& (*Manip)(std::ostream&);
  Message& operator<<(Manip manip) {
    ss << manip;
    return *this;
  }
};

template <class C> struct Stream {};
template <class C> Stream<C>& identity(Stream<C>& s) { return s; }

struct Sink {
  int templates = 0;
  int pointers = 0;
  template <typename T> Sink& take(const T&) {
    templates++;
    return *this;
  }
  Sink& take(Stream<char>& (*)(Stream<char>&)) {
    pointers++;
    return *this;
  }
};

int main() {
  Message m;
  m << "a" << std::endl << "b" << std::ends << std::flush << 'c';
  std::string s = m.ss.str();
  if (s != std::string("a\nb\0c", 5)) return 1;

  Message& r = m << std::endl;
  if (&r != &m) return 2;

  Sink sink;
  sink.take(identity).take(&identity).take(1);
  if (sink.pointers != 2 || sink.templates != 1) return 3;
  return 0;
}
