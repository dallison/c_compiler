// RUN: -std=c++11
// EXPECT: Illegal global storage specified: register

register thread_local int value;
