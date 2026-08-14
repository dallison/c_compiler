// RUN: -std=c++26
// EXPECT: non-reference parameter 'value' used by a postcondition must have const type

int invalid_post(int value)
    post (value > 0) {
  return value;
}
