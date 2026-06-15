// RUN: -std=c++17
// EXPECT: dynamic_cast is not supported yet
int main(void) {
  int value = 7;
  int* ptr = &value;
  int* other = dynamic_cast<int*>(ptr);
  return sizeof(other);
}
