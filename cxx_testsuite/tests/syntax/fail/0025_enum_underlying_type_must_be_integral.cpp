// RUN: -std=c++17
// EXPECT: Enum underlying type must be integral
enum class Bad : float {
  value,
};
