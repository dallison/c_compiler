// RUN: -std=c++17
struct Base {
  int value;
};

struct Derived : public Base {
  int extra;
};

int main(void) {
  Derived derived;
  Derived* derived_ptr = &derived;
  Base* base_ptr = dynamic_cast<Base*>(derived_ptr);
  Derived* same_ptr = dynamic_cast<Derived*>(derived_ptr);
  return sizeof(base_ptr) + sizeof(same_ptr);
}
