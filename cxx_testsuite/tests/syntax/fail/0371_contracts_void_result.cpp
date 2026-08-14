// RUN: -std=c++26
// EXPECT: a void function cannot declare a postcondition result

void invalid_result()
    post (result: true) {
}
