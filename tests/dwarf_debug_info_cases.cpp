extern "C" int add(int a, int b) {
  int sum = a + b;
  return sum;
}

struct Point {
  int x;
  int y;
};

enum Color { RED = 1, GREEN = 2 };

extern "C" int g;

extern "C" int main() {
  Point p;
  p.x = RED;
  p.y = g;
  return add(p.x, p.y);
}
