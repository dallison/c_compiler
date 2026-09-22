// RUN: -std=c++20
// EXPECT: No such symbol "not_declared"
// EXPECT: expanded from macro 'BAD'
// EXPECT: #define BAD not_declared

#define BAD not_declared

int x = BAD;
