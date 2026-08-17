// RUN: -std=c++26
// EXPECT: deleting a pointer to an incomplete class type is not allowed in C++26

struct Incomplete;

void destroy(Incomplete* value) {
  delete value;
}
