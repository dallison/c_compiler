// RUN: -std=c++20
// EXPECT: Illegal pointer conversion

// [conv.qual]/3: adding const at level 2 requires const at every level above
// it, so `char**` does not convert to `const char**`.
char* text;
char** source = &text;
const char** destination = source;
