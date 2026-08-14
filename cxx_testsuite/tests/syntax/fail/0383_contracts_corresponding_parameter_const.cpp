// RUN: -std=c++26
// EXPECT: non-reference parameter 'value' used by a postcondition must have const type

int corresponding_const(const int value)
    post (value >= 0);

int corresponding_const(int value) {
  return value;
}
