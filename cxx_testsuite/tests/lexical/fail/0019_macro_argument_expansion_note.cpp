// RUN: -std=c++20
// EXPECT: No such symbol "not_declared"
// EXPECT: expanded from macro 'ADD'
// EXPECT: expanded from macro 'FOO'
// EXPECT: #define ADD(x) x
// EXPECT: #define FOO not_declared

#define FOO not_declared
#define ADD(x) x

int x = ADD(FOO);
