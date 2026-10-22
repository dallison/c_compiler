// RUN: -std=c++17
// EXPECT_EXIT: 0
// std::nullptr_t converts to a pointer to member function, including as a
// constructor argument (protobuf ClassData's clear slot).

struct Base {
  void clear() {}
};

struct Slot {
  void (Base::*clear)();
  explicit Slot(void (Base::*fn)()) : clear(fn) {}
};

int main() {
  void (Base::*direct)() = nullptr;
  Slot slot(nullptr);
  if (direct != nullptr || slot.clear != nullptr) {
    return 1;
  }
  direct = &Base::clear;
  if (direct == nullptr) {
    return 2;
  }
  return 0;
}
