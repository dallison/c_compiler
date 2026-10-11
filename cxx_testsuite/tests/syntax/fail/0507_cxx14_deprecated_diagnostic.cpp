// RUN: -std=c++14 -Werror=deprecated-declarations
// EXPECT: 'old_function' is deprecated: use replacement
// EXPECT: 'old_function' was marked deprecated here
// EXPECT: 'old_gnu_function' is deprecated: use newer GNU function
// EXPECT: 'old_gnu_function' was marked deprecated here
// EXPECT: return old_function() + old_gnu_function();
// EXPECT: ^^^^^^^^^^^^^^^^
// EXPECT: int old_gnu_function() __attribute__((deprecated("use newer GNU function")));
// EXPECT: ^^^^^^^^^^

[[deprecated("use replacement")]] int old_function() {
  return 1;
}

int old_gnu_function() __attribute__((deprecated("use newer GNU function")));
int old_gnu_function() {
  return 2;
}

int use_old_function() {
  return old_function() + old_gnu_function();
}
