// RUN: -std=c++20
// EXPECT_EXIT: 0

// The address of a non-const global is a constant, but pointers stored in it
// are not: comparisons that read them must happen at run time, after main has
// changed them.

struct S {
  char* p;
  char buf[16];
  S() : p(buf) {}
};

struct Named {
  S name;
  int k;
};

struct Ptrs {
  int* ptrs[2];
};

Named global_named{};
int target[2];
int* global_ptrs[2] = {&target[0], &target[1]};
Ptrs global_struct = {{&target[0], &target[1]}};
Named* const named_ptr = &global_named;

int main() {
  if (global_named.name.p != global_named.name.buf) return 1;
  if (global_ptrs[1] != &target[1]) return 2;
  if (global_struct.ptrs[1] != &target[1]) return 3;
  if (named_ptr->name.p != global_named.name.buf) return 4;
  if (&named_ptr->name != &global_named.name) return 5;

  global_named.name.p = nullptr;
  global_ptrs[1] = &target[0];
  global_struct.ptrs[1] = nullptr;
  if (global_named.name.p == global_named.name.buf) return 6;
  if (global_ptrs[1] == &target[1]) return 7;
  if (global_struct.ptrs[1] == &target[1]) return 8;
  if (named_ptr->name.p == global_named.name.buf) return 9;
  return 0;
}
