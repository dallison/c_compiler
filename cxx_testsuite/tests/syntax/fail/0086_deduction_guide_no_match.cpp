// RUN: -std=c++20

template <typename T>
struct Holder {
  T value;
};

int main(void) {
  Holder missing;
  return 0;
}
