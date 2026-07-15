export module hello;

export int module_answer() {
  return 42;
}

export namespace greet {
export constexpr int value = 7;
}

int module_hidden() {
  return 7;
}
