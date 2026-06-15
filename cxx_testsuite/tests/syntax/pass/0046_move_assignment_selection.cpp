// RUN: -std=c++20

struct Box {
  int value;
  Box(int initial);
  int operator=(const Box& other);
  int operator=(Box&& other);
};

Box::Box(int initial) {
  value = initial;
}

int Box::operator=(const Box& other) {
  value = other.value + 10;
  return value;
}

int Box::operator=(Box&& other) {
  value = other.value + 20;
  return value;
}

int main(void) {
  Box source(1);
  Box target(0);
  target = source;
  target = static_cast<Box&&>(source);
  return target.value;
}

struct MoveOnly {
  MoveOnly(int initial);
  int operator=(MoveOnly&& other);

private:
  int operator=(const MoveOnly& other);
};

MoveOnly make_move_only(void);

int from_prvalue(void) {
  MoveOnly target(0);
  target = make_move_only();
  return 0;
}
