// RUN: -std=c++26
// EXPECT: usual arithmetic conversions between different enumeration types

enum First { first };
enum Second { second };

int combine(First lhs, Second rhs) {
  return lhs + rhs;
}

double scale(First lhs, double rhs) {
  return lhs * rhs;
}
