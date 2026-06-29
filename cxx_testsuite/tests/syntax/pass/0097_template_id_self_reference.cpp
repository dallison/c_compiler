// RUN: -std=c++20

// A class template may refer to itself by its own template-id (e.g. Box<T> or
// Box<int>) inside its own body, including in member-function and friend
// signatures - not only as a member-variable type.

template <typename T>
struct Box {
  T value;

  // Member-variable types (already supported).
  Box<T>* self_t;
  Box<int>* self_int;

  // Member-function prototypes referencing the current template-id.
  void take_t(const Box<T>& other);
  void take_int(const Box<int>& other);

  // Friend signatures referencing the current template-id.
  friend void touch_t(const Box<T>& b);
  friend void touch_int(const Box<int>& b);
};

template <typename T>
void Box<T>::take_t(const Box<T>& other) {
  value = other.value;
}

int main(void) { return 0; }
