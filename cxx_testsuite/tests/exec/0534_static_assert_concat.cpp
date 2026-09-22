// RUN: -std=c++17
// EXPECT_EXIT: 0

static_assert(true, "hello " "world");
static_assert("lts"[0] != '\0', "namespace "
                                "must not be empty");

int main() { return 0; }
