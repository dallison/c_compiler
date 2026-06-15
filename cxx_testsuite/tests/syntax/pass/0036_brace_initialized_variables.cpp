// RUN: -std=c++17
int global_value{3};

struct Pair {
  int left;
  int right;
};

class Box {
 public:
  Box(int value);
  int value;
};

Box::Box(int init) {
  value = init;
}

int main(void) {
  int local_value{4};
  Pair pair{1, 2};
  Box box{5};
  return global_value + local_value + pair.left + pair.right + box.value;
}
