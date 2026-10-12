// RUN: -std=c++17
// EXPECT_EXIT: 0

enum Token { A, B, C, D, E, F, G, H };

int dispatch(Token token) {
  goto table;

table:
  switch (token) {
    case A:
      return 10;
    case B:
      return 11;
    case C:
      return 12;
    case D:
      return 13;
    case E:
      return 14;
    case F:
      return 15;
    case G:
      return 16;
    case H:
      return 17;
  }
}

int main() { return dispatch(C) == 12 ? 0 : 1; }
