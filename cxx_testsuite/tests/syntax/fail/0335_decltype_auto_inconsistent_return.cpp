// RUN: -std=c++20
// EXPECT: Inconsistent auto function return type

int value;

decltype(auto) inconsistent_decltype_auto_return(bool copy) {
  if (copy) {
    return value;
  }
  return (value);
}
