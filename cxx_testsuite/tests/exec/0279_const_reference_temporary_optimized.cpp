// RUN: -std=c++20 -O1
// EXPECT_EXIT: 0

long long read_value(const long long& value) {
  return value;
}

int main() {
  return read_value(42) == 42 ? 0 : 1;
}
