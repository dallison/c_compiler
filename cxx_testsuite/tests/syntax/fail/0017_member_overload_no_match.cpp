// RUN: -std=c++17
// EXPECT: No matching overload for pick
struct Box {
  int pick(int *p);
  int pick(char *p);
};

int main(void) {
  Box box;
  return sizeof(box.pick(1));
}
