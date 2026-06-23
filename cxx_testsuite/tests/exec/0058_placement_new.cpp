// RUN: -std=c++17
void* operator new(unsigned long size, void* ptr) {
  return ptr;
}

struct Box {
  int value;
  int* destroyed;

  Box(int v) : value(v + 1), destroyed(0) {
  }

  Box(int v, int* d) : value(v + 1), destroyed(d) {
  }

  ~Box() {
    if (destroyed != 0) {
      *destroyed += value;
    }
  }
};

struct Pair {
  Box first;
  Box second;

  Pair(int* destroyed) : first(1, destroyed), second(2, destroyed) {
  }
};

int main(void) {
  unsigned long storage[4];
  Box* box = new ((void*)storage) Box(41);
  if (box != (Box*)storage) {
    return 1;
  }
  if (box->value != 42) {
    return 6;
  }

  unsigned long int_storage[1];
  int* value = new ((void*)int_storage) int(7);
  if (value != (int*)int_storage) {
    return 2;
  }
  if (*value != 7) {
    return 3;
  }

  int destroyed = 0;
  Box* explicit_box = new ((void*)storage) Box(5, &destroyed);
  explicit_box->~Box();
  if (destroyed != 6) {
    return 4;
  }

  Box* dot_box = new ((void*)storage) Box(7, &destroyed);
  (*dot_box).~Box();
  if (destroyed != 14) {
    return 5;
  }

  unsigned long array_storage[8];
  Box* boxes = (Box*)array_storage;
  Box* first = new ((void*)&boxes[0]) Box(3, &destroyed);
  Box* second = new ((void*)&boxes[1]) Box(4, &destroyed);
  first->~Box();
  second->~Box();
  if (destroyed != 23) {
    return 7;
  }

  unsigned long pair_storage[8];
  Pair* pair = new ((void*)pair_storage) Pair(&destroyed);
  pair->second.~Box();
  new ((void*)&pair->second) Box(9, &destroyed);
  pair->second.~Box();
  pair->first.~Box();
  if (destroyed != 38) {
    return 8;
  }

  {
    Pair scoped_pair(&destroyed);
  }
  if (destroyed != 43) {
    return 9;
  }

  return 0;
}
