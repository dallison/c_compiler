// RUN: -std=c++17
struct Left {
  int left;
  int left_value(void);
};

int Left::left_value(void) {
  return left;
}

struct Right {
  int right;
  int right_value(void);
};

int Right::right_value(void) {
  return right;
}

struct Derived : public Left, public Right {
  int derived;
};

int read_left(Left* left) {
  return left->left;
}

int read_right(Right* right) {
  return right->right;
}

int main(void) {
  Derived derived;
  derived.left = 3;
  derived.right = 5;
  derived.derived = 7;

  Left* left = &derived;
  Right* right = &derived;
  return sizeof(Derived) + read_left(left) + read_right(right) +
         derived.left_value() + derived.right_value() + derived.derived;
}
