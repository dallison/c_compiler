// RUN: -std=c++26

#include <memory>
#include <type_traits>

struct tracked {
  static int destructions;
  int value;

  explicit tracked(int input) : value(input) {}
  ~tracked() { ++destructions; }
};

int tracked::destructions;

struct throwing {
  static int destructions;

  throwing() { throw 7; }
  ~throwing() { ++destructions; }
};

int throwing::destructions;

union storage {
  tracked object;
  int numbers[2];
  int fallback;
};

union nested_storage {
  storage inner;
  int fallback;
};

struct anonymous_storage {
  union {
    tracked object;
    int fallback;
  };
};

template <class T>
int classify(T&) requires std::is_trivially_destructible_v<T> {
  return 1;
}

template <class T>
int classify(T&) {
  return 0;
}

int main() {
  static_assert(std::is_trivially_default_constructible_v<storage>);
  static_assert(std::is_trivially_destructible_v<storage>);

  {
    storage value;
    std::construct_at(&value.object, 42);
    if (value.object.value != 42 || classify(value) != 1) {
      return 1;
    }
  }
  if (tracked::destructions != 0) {
    return 2;
  }

  {
    storage value;
    std::start_lifetime(value.numbers);
    std::construct_at(&value.numbers[1], 22);
    if (value.numbers[1] != 22) {
      return 3;
    }
  }
  if (tracked::destructions != 0) {
    return 4;
  }

  storage manually_destroyed;
  std::construct_at(&manually_destroyed.object, 7);
  std::destroy_at(&manually_destroyed.object);
  if (tracked::destructions != 1) {
    return 5;
  }

  {
    nested_storage value;
    std::start_lifetime(value.inner);
    std::construct_at(&value.inner.object, 9);
    if (value.inner.object.value != 9) {
      return 6;
    }
  }
  if (tracked::destructions != 1) {
    return 7;
  }

  {
    anonymous_storage value;
    std::construct_at(&value.object, 11);
    if (value.object.value != 11) {
      return 8;
    }
  }
  if (tracked::destructions != 1) {
    return 9;
  }

  union exceptional_storage {
    throwing object;
    int fallback;
  };

  try {
    exceptional_storage value;
    std::construct_at(&value.object);
    return 10;
  } catch (int error) {
    if (error != 7 || throwing::destructions != 0) {
      return 11;
    }
  }
  return 0;
}
