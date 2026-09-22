// RUN: -std=c++20
// EXPECT: No such symbol "not_declared"
// EXPECT: expanded from macro 'OUTER'
// EXPECT: expanded from macro 'INNER'
// EXPECT: #define INNER not_declared
// EXPECT: #define OUTER INNER

#define INNER not_declared
#define OUTER INNER

int x = OUTER;
