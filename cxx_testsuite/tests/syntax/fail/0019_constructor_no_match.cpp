// RUN: -std=c++17
// EXPECT: No matching overload for Box
struct Box {
  Box(int *p);
};

int main(void) {
  Box box(1);
  return 0;
}
