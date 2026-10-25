// RUN: -target wasm32 -std=c++20
// EXPECT: <condition_variable> is unavailable in this single-threaded target profile

#include <condition_variable>
