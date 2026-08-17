// RUN: -std=c++26
// EXPECT: token paste result '**' is not a valid preprocessing token

#define JOIN(left, right) left ## right

int invalid = JOIN(*, *);
