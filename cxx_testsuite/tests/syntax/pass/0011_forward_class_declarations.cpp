// RUN: -std=c++17
class Forward;

Forward* forward_pointer;
class Forward {
  int value;
};

Forward value;

int main(void) {
  return 0;
}
