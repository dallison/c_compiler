// RUN: -std=c++20
// EXPECT_EXIT: 0
// Declarations that only name an incomplete class, and classes completed
// before they are needed, are not diagnosed.

struct Later;
extern Later later_decl;
Later* later_ptr;
Later& LaterRef();
void TakesLater(Later);
Later ReturnsLater();
struct Later {
  int value;
};
Later later_decl;
void TakesLater(Later) {}
Later ReturnsLater() { return Later{}; }

template <class T>
struct Box {
  T* ptr;
};
struct Never;
Box<Never> never_box;
Box<Never>* never_box_ptr;

struct Outer {
  struct Nested;
  Nested* nested_ptr;
  struct Nested {
    int x;
  };
  Nested nested;
  // Member function bodies and parameters see the complete class.
  void Set(Nested n) { nested = n; }
  struct Deferred;
  void Use(Deferred d);
  struct Deferred {
    int y;
  };
  static Outer instance;
  Outer Copy() { return *this; }
  friend Outer Clone(Outer o) { return o; }
};
Outer Outer::instance;
void Outer::Use(Deferred d) { nested.x = d.y; }

struct Enclosing {
  struct Member;
  int z;
};
struct Enclosing::Member {
  Enclosing enclosing;
};

int main() {
  struct Local {
    Local* next;
  } local{nullptr};
  try {
    throw Later{1};
  } catch (Later& l) {
    return l.value - 1 + (local.next == nullptr ? 0 : 1);
  } catch (Later*) {
  }
  return 1;
}
