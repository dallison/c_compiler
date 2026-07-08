// RUN: -std=c++17
// Template template parameters are parsed and diagnosed cleanly rather than
// crashing the compiler.  Full support (using `C<...>` as a template-id and
// binding a class-template argument) is not yet implemented.
// EXPECT: template template parameters are not yet supported

template <template <class> class C>
struct B {};

int main() { return 0; }
