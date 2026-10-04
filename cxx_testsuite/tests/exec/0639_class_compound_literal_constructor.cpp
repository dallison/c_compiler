// RUN: -std=c++20
// EXPECT_EXIT: 0

// A compound literal of a non-aggregate class (`(P){P(5)}`, a GNU extension)
// is initialized by its constructor from the braced value, not member-wise.
struct P {
  int v[4];
  P(int x) {
    for (int i = 0; i < 4; ++i) v[i] = x + i;
  }
  P(P&& o) {
    for (int i = 0; i < 4; ++i) v[i] = o.v[i];
  }
};

struct Q {
  int v[4];
  Q(int x) {
    for (int i = 0; i < 4; ++i) v[i] = x + i;
  }
};

int main() {
  P t = (P){P(5)};
  Q u = (Q){Q(5)};
  return (t.v[2] == 7 ? 0 : 1) + (u.v[2] == 7 ? 0 : 2);
}
