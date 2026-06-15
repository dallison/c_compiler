// RUN: -std=c++20

struct Box {
  int value;
  Box(int initial);
  Box(const Box& other);
  Box(Box&& other);
};

Box::Box(int initial) {
  value = initial;
}

Box::Box(const Box& other) {
  value = other.value + 10;
}

Box::Box(Box&& other) {
  value = other.value + 20;
}

int main(void) {
  Box source(1);
  Box copied(source);
  Box moved(static_cast<Box&&>(source));
  return copied.value + moved.value;
}

struct MoveOnly {
  MoveOnly(int initial);
  MoveOnly(MoveOnly&& other);

private:
  MoveOnly(const MoveOnly& other);
};

MoveOnly make_move_only(void);

int from_prvalue(void) {
  MoveOnly moved(make_move_only());
  return 0;
}
