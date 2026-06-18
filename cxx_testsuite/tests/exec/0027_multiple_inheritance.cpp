int trace;

struct Left {
  int left;
  int left_value(void);
};

int Left::left_value(void) {
  return left + 1;
}

struct Right {
  int right;
  int right_value(void);
};

int Right::right_value(void) {
  return right + 2;
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

void write_right(Right* right, int value) {
  right->right = value;
}

struct ConstructLeft {
  ConstructLeft();
  ~ConstructLeft();
};

ConstructLeft::ConstructLeft() {
  if (trace != 0) {
    trace = 100;
    return;
  }
  trace = 1;
}

ConstructLeft::~ConstructLeft() {
  if (trace != 5) {
    trace = 200;
    return;
  }
  trace = 6;
}

struct ConstructRight {
  ConstructRight();
  ~ConstructRight();
};

ConstructRight::ConstructRight() {
  if (trace != 1) {
    trace = 101;
    return;
  }
  trace = 2;
}

ConstructRight::~ConstructRight() {
  if (trace != 4) {
    trace = 201;
    return;
  }
  trace = 5;
}

struct ConstructDerived : public ConstructLeft, public ConstructRight {
  ConstructDerived();
  ~ConstructDerived();
};

ConstructDerived::ConstructDerived() {
  if (trace != 2) {
    trace = 102;
    return;
  }
  trace = 3;
}

ConstructDerived::~ConstructDerived() {
  if (trace != 3) {
    trace = 202;
    return;
  }
  trace = 4;
}

int main(void) {
  Derived derived;
  derived.left = 11;
  derived.right = 17;
  derived.derived = 23;

  if (sizeof(Derived) != sizeof(Left) + sizeof(Right) + sizeof(int)) {
    return 1;
  }
  if (derived.left != 11) {
    return 2;
  }
  if (derived.right != 17) {
    return 3;
  }
  if (derived.left_value() != 12) {
    return 4;
  }
  if (derived.right_value() != 19) {
    return 5;
  }
  if (read_left(&derived) != 11) {
    return 6;
  }
  if (read_right(&derived) != 17) {
    return 7;
  }

  Right* right = &derived;
  right->right = 29;
  if (derived.right != 29) {
    return 8;
  }
  write_right(&derived, 31);
  if (right->right != 31) {
    return 9;
  }

  ConstructDerived* constructed = new ConstructDerived;
  if (trace != 3) {
    return trace;
  }
  delete constructed;
  if (trace != 6) {
    return trace;
  }

  return 0;
}
