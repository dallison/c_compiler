// RUN: -std=c++17
// EXPECT_EXIT: 0

struct O {
  template <class T>
  O& put(T v) {
    last = (int)v;
    return *this;
  }

  O& write(int v) { return put(static_cast<long>(v)); }

  int last;
};

int main() {
  O o;
  o.last = 0;
  o.write(7);
  return o.last == 7 ? 0 : 1;
}
