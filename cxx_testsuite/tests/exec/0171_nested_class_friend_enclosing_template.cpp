// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct Box {
  class nested {
   public:
    explicit nested(T v) : value(v) {}

   private:
    T value;

    friend class Box;
  };

  nested make(T value) const {
    return nested(value);
  }

  T read(const nested& value) const {
    return value.value;
  }
};

int main() {
  Box<int> box;
  Box<int>::nested value = box.make(42);
  return box.read(value) == 42 ? 0 : 1;
}
