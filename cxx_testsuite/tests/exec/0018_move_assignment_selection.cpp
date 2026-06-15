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
  if (target.value != 11) {
    return 1;
  }

  target = static_cast<Box&&>(source);
  if (target.value != 21) {
    return 2;
  }

  const Box const_source(3);
  target = const_source;
  if (target.value != 13) {
    return 3;
  }

  return 0;
}
