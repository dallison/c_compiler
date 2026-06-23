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

Box make_local(void) {
  Box local(1);
  return local;
}

Box make_direct(void) {
  return Box(2);
}

int main(void) {
  Box from_local(make_local());
  if (from_local.value != 1) {
    return from_local.value;
  }

  Box from_direct(make_direct());
  if (from_direct.value != 2) {
    return from_direct.value;
  }
  return 0;
}
