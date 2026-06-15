// RUN: -std=c++17
// EXPECT: redeclared with different underlying type
enum class Mismatch : unsigned char;
enum class Mismatch : unsigned int {
  value,
};
