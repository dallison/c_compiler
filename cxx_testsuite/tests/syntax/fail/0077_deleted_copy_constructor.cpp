// RUN: -std=c++20

struct NoCopy {
  int value;

  NoCopy() = default;
  NoCopy(const NoCopy& other) = delete;
};

int main(void) {
  NoCopy first;
  NoCopy second(first);
  return 0;
}
