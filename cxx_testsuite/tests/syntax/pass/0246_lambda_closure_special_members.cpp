// RUN: -std=c++20
#include <typeinfo>

void* operator new(unsigned long, void* ptr) {
  return ptr;
}

struct Tracked {
  int value;
  int* destroyed;

  Tracked(int v, int* d) : value(v), destroyed(d) {
    if (destroyed != 0) {
      *destroyed += 1;
    }
  }

  Tracked(const Tracked& other) : value(other.value), destroyed(other.destroyed) {
    if (destroyed != 0) {
      *destroyed += 10;
    }
  }

  Tracked(Tracked&& other) : value(other.value), destroyed(other.destroyed) {
    other.destroyed = 0;
    if (destroyed != 0) {
      *destroyed += 20;
    }
  }

  ~Tracked() {
    if (destroyed != 0) {
      *destroyed += 100 + value;
    }
  }
};

struct MoveOnly {
  int value;
  MoveOnly(int v) : value(v) {
  }
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&& other) : value(other.value) {
  }
};

int main(void) {
  auto captureless = [] {
    return 1;
  };
  decltype(captureless) defaulted{};
  decltype(captureless) copied = captureless;
  decltype(captureless) moved = static_cast<decltype(captureless)&&>(copied);
  defaulted = captureless;

  int base = 4;
  auto value_capture = [base](int extra) {
    return base + extra;
  };
  auto value_copy = value_capture;

  int ref = 2;
  auto ref_capture = [&ref] {
    ref = 9;
    return ref;
  };
  auto ref_copy = ref_capture;

  int destroys = 0;
  auto init_capture = [boxed = Tracked(3, &destroys)] {
    return boxed.value;
  };
  auto init_copy = init_capture;

  unsigned long storage[8];
  Tracked source(5, &destroys);
  Tracked* placed_copy = new ((void*)storage) Tracked(source);
  placed_copy->~Tracked();

  MoveOnly move_only(7);
  MoveOnly* placed_move =
      new ((void*)storage) MoveOnly(static_cast<MoveOnly&&>(move_only));
  placed_move->~MoveOnly();

  const std::type_info& plain_ti = typeid(decltype(captureless));
  const std::type_info& value_ti = typeid(decltype(value_capture));
  if (plain_ti == value_ti) {
    return 1;
  }

  return captureless() + defaulted() + copied() + moved() + value_copy(1) +
         ref_copy() + init_copy();
}
