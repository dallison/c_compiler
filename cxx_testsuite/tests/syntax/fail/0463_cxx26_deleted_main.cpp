// RUN: -std=c++26
// EXPECT: 'main' cannot be defined as deleted

int main() = delete;
