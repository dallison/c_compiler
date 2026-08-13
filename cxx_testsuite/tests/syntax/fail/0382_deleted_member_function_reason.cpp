// RUN: -std=c++26
// EXPECT: call reset() without an argument

struct controller {
  void reset(int) = delete("call reset() without an argument");
};

int main() {
  controller value;
  value.reset(3);
  return 0;
}
