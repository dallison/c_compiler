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
  if (copied.value != 11) {
    return 1;
  }

  Box moved(static_cast<Box&&>(source));
  if (moved.value != 21) {
    return 2;
  }

  const Box const_source(3);
  Box copied_const(const_source);
  if (copied_const.value != 13) {
    return 3;
  }

  return 0;
}
