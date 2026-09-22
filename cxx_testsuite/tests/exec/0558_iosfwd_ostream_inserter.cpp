// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class T>
class basic_ostream;

using ostream = basic_ostream<char>;

struct Value {
  int n;
};

ostream& operator<<(ostream& os, Value v);
void write(ostream& os, int n);

template <class T>
class basic_ostream {
 public:
  T* p;
  int last;
  basic_ostream() : p(0), last(0) {}
};

ostream& operator<<(ostream& os, Value v) {
  os.last = v.n;
  return os;
}

void write(ostream& os, int n) {
  os.last = n;
}

int main() {
  ostream os;
  Value v;
  v.n = 7;
  operator<<(os, v);
  if (os.last != 7) {
    return 1;
  }
  write(os, 9);
  if (os.last != 9) {
    return 2;
  }
  return 0;
}
