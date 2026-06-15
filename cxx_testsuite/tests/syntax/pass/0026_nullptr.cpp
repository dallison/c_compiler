// RUN: -std=c++17
int accepts_pointer(int *p) {
  return sizeof(p);
}

int main(void) {
  int *p = nullptr;
  p = nullptr;
  return sizeof(nullptr) + sizeof(p == nullptr) + sizeof(accepts_pointer(nullptr));
}
