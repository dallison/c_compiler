// RUN: -std=c++26
// EXPECT: Type splice operand does not reflect a type

int global_object = 0;
constexpr auto not_a_type = ^^global_object;

typename[:not_a_type:] value = 0;
