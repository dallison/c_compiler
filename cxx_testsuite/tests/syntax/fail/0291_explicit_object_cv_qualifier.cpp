// RUN: -std=c++23
// EXPECT: explicit object member function cannot have cv or ref qualifiers

struct value {
  void invalid(this value& self) const;
};
