// RUN: -std=c++11
// Nested type definitions in an enumerator initializer are ill-formed.
// Recovery must not skip the enclosing '}' and spin in the class-member loop.
struct PR28903 {
  enum {
    PR28903_A = (enum {
      PR28903_B,
      PR28903_C = PR28903_B
    })
  };
};

enum {
  A = (enum { B, C = B })
};
