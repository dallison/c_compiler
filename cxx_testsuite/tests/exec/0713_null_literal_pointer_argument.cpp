// RUN: -std=c++20
// EXPECT_EXIT: 0

// A literal 0 passed for a pointer parameter, explicitly or as a default
// argument, is a full-width null pointer even when the stack slot it lands in
// previously held nonzero bytes.

struct Cap {
  long value;
};

__attribute__((noinline)) static bool IsSet(const char* text, bool flag,
                                            unsigned mode, Cap* caps = 0,
                                            void* extra = 0) {
  (void)text;
  (void)flag;
  (void)mode;
  return caps != nullptr || extra != nullptr;
}

template <class T>
struct Engine {
  __attribute__((noinline)) static bool Find(const T* text, bool flag,
                                             unsigned mode, Cap* caps = 0) {
    return IsSet(text, flag, mode, caps);
  }
};

__attribute__((noinline)) static void Dirty() {
  volatile long junk[64];
  for (int i = 0; i < 64; i++) {
    junk[i] = -1;
  }
}

int main() {
  Dirty();
  if (IsSet("a", true, 1)) {
    return 1;
  }
  Dirty();
  if (IsSet("a", true, 1, 0, 0)) {
    return 2;
  }
  Dirty();
  if (Engine<char>::Find("a", true, 1)) {
    return 3;
  }
  Cap cap{4};
  Dirty();
  if (!IsSet("a", true, 1, &cap)) {
    return 4;
  }
  return 0;
}
