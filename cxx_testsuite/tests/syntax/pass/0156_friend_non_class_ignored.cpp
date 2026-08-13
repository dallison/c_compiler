// RUN: -std=c++20
//
// A friend type specifier that does not designate a class is ignored.

struct Owner {
  friend int;
};

template <class T>
struct DependentOwner {
  friend T;
};

DependentOwner<int> dependent_owner;

int main(void) {
  Owner owner;
  (void)owner;
  (void)dependent_owner;
  return 0;
}
