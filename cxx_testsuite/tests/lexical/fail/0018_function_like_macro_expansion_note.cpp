// RUN: -std=c++20
// EXPECT: No such symbol "not_declared"
// EXPECT: expanded from macro 'ID'
// EXPECT: #define ID(x) x

#define ID(x) x

int x = ID(not_declared);
