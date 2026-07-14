// RUN: -std=c++20
// EXPECT: Only == and != are valid for pointers to members

struct Object {
  int first;
  int second;
};

int main(void) {
  int Object::*first = &Object::first;
  int Object::*second = &Object::second;
  return first < second;
}
