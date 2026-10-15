// A typedef formed while its class template is only forward-declared (the
// <iosfwd> `typedef basic_ostream<char> ostream` pattern) must still name the
// base subobject when a derived pointer or reference converts to it.
template <class C> struct Traits {};
template <class C, class T = Traits<C>> struct Ios;
template <class C, class T = Traits<C>> struct Out;
template <class C, class T = Traits<C>> struct Stream;
typedef Out<char> OutT;
typedef Stream<char> StreamT;

template <class C, class T> struct Ios {
  long state = 0;
  virtual ~Ios() {}
};
template <class C, class T = Traits<C>> struct In : virtual Ios<C, T> {
  long a = 1;
};
template <class C, class T> struct Out : virtual Ios<C, T> {
  long b = 2;
};
template <class C, class T> struct Stream : In<C, T>, Out<C, T> {
  long d = 3;
};

static OutT* seen = nullptr;
void Take(OutT* out) { seen = out; }
long ReadB(OutT& out) { return out.b; }

struct Listener {
  explicit Listener(OutT* out) : out(out) {}
  OutT* out;
};

int main() {
  StreamT stream;
  stream.b = 42;
  OutT* expected = &stream;
  if ((void*)expected == (void*)&stream) return 1;
  if (expected->b != 42) return 2;

  Take(&stream);
  if (seen != expected) return 3;
  Take(static_cast<OutT*>(&stream));
  if (seen != expected) return 4;
  Listener listener(&stream);
  if (listener.out != expected) return 5;
  if (ReadB(stream) != 42) return 6;
  return 0;
}
