// RUN: -std=c++20

[[gnu::vector_size(16)]] int vector_int;

int main() {
  vector_int = {};
  return 0;
}
