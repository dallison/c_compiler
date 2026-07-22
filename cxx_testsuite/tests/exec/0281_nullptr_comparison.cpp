// RUN: -std=c++20

int main() {
  decltype(nullptr) left = nullptr;
  decltype(nullptr) right = nullptr;

  if (!(left == right)) {
    return 1;
  }
  if (left != right) {
    return 2;
  }
  return 0;
}
