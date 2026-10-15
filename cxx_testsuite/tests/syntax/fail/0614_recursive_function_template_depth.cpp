// RUN: -std=c++20 -ftemplate-depth=64
// EXPECT: recursive template instantiation exceeded maximum depth of 64

// Each deferred body instantiates f<T*>, so the chain never ends.
template <class T> int f(T t) { return f(&t); }

int main() { return f(0); }
