// RUN: -std=c++20

struct NonCopyable {
  int value;

  NonCopyable(int initial);
  NonCopyable(const NonCopyable& other) = delete;
  NonCopyable(NonCopyable&& other) = delete;
};

NonCopyable::NonCopyable(int initial) {
  value = initial;
}

NonCopyable make_direct_noncopyable(void) {
  return NonCopyable(1);
}

NonCopyable make_local_noncopyable(void) {
  NonCopyable local(2);
  return local;
}

int use_noncopyable_elision(void) {
  return 0;
}
