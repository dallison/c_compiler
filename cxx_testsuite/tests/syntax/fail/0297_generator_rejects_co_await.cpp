// RUN: -std=c++23
// EXPECT: No viable overload for await_transform

#include <coroutine>
#include <generator>

std::generator<int> invalid() {
  co_await std::suspend_always{};
  co_yield 1;
}
