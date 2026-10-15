// RUN: -std=c++20
// EXPECT: invalid application of 'sizeof' to a function type

// `sizeof(T())` names the function type `T()`, not a value-initialized T.
template <class T> int SizeOfCall() { return sizeof(T()); }

int main() { return SizeOfCall<int>(); }
