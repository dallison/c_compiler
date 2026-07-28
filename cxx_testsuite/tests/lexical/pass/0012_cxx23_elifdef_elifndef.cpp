// RUN: -std=c++23

#define PRESENT 1
#define REMOVED 1
#undef REMOVED

#ifndef REMOVED
int selected_ordinary_ifndef;
#else
#error "#ifndef treated an undefined macro as defined"
#endif

#if 0
#error "initial false branch selected"
#elifdef PRESENT
int selected_present;
#else
#error "#elifdef did not select a defined macro"
#endif

#if 0
#elifndef ABSENT
int selected_absent;
#else
#error "#elifndef did not select an undefined macro"
#endif

#if 1
int selected_initial;
#elifdef PRESENT
#error "#elifdef selected after an earlier branch"
#elifndef ABSENT
#error "#elifndef selected after an earlier branch"
#else
#error "#else selected after an earlier branch"
#endif

#if 0
#elifdef REMOVED
#error "#elifdef treated an undefined macro as defined"
#elifndef REMOVED
int selected_removed;
#else
#error "#elifndef treated an undefined macro as defined"
#endif

#if 0
#elifdef __has_include
int selected_builtin;
#else
#error "__has_include must be treated as a defined macro"
#endif

int main(void) {
  return 0;
}
