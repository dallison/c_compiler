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
  if (defaulted() != 1 || copied() != 1 || moved() != 1) {
    return 1;
  }

  int base = 4;
  auto value_capture = [base](int extra) {
    return base + extra;
  };
  auto value_copy = value_capture;
  if (value_copy(2) != 6) {
    return 2;
  }

  int ref = 2;
  auto ref_capture = [&ref] {
    ref = 9;
    return ref;
  };
  auto ref_copy = ref_capture;
  if (ref_copy() != 9 || ref != 9) {
    return 3;
  }

  int destroys = 0;
  {
    auto init_capture = [boxed = Tracked(3, &destroys)] {
      return boxed.value;
    };
    if (init_capture() != 3) {
      return 4;
    }
    auto init_copy = init_capture;
    if (init_copy() != 3) {
      return 5;
    }
  }
  if (destroys != 217) {
    return 6;
  }

  unsigned long storage[8];
  destroys = 0;
  Tracked source(5, &destroys);
  Tracked* placed_copy = new ((void*)storage) Tracked(source);
  if (placed_copy->value != 5) {
    return 7;
  }
  placed_copy->~Tracked();
  if (destroys != 116) {
    return 8;
  }

  MoveOnly move_only(7);
  MoveOnly* placed_move =
      new ((void*)storage) MoveOnly(static_cast<MoveOnly&&>(move_only));
  if (placed_move->value != 7) {
    return 9;
  }
  placed_move->~MoveOnly();

  auto rtti_a = [] {
    return 1;
  };
  auto rtti_b = [x = 2](int y) {
    return x + y;
  };
  if (typeid(decltype(rtti_a)) == typeid(decltype(rtti_b))) {
    return 10;
  }
  if (typeid(decltype(rtti_a)) != typeid(decltype(rtti_a))) {
    return 11;
  }

  return 0;
}
