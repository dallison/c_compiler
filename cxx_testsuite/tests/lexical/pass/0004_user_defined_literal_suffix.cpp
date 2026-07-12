// RUN: -std=c++11
constexpr unsigned long long operator""_km(unsigned long long value) {
  return value;
}

int main(void) {
  return 123_km == 123 ? 0 : 1;
}
