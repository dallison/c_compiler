// RUN: -std=c++20
// EXPECT: Illegal conversion; cannot convert from 'int' to 'int*'

// Since C++11 only an integer literal is a null pointer constant (CWG 903).
void take(int*);

int main() {
  take(int());
  return 0;
}
