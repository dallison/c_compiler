// RUN: -std=c++20
// EXPECT: 'vector_size' type attribute is not supported

[[gnu::vector_size(16)]] int vector_int;
