// RUN: -std=c++20
// EXPECT: Template non-type argument is not compatible with parameter type

// A reference template parameter binds only to an object of its referenced
// type with no more cv-qualification.
const int cg = 2;
template <int& R>
int Ref() { return R; }

int main() { return Ref<cg>(); }
