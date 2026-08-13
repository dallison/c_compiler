// RUN: -std=c++26
// EXPECT: converted

struct message_size {
  constexpr operator unsigned() const { return 9; }
};

struct message_data {
  constexpr operator const char*() const { return "converted"; }
};

struct message {
  constexpr message_data data() const { return {}; }
  constexpr message_size size() const { return {}; }
};

static_assert(false, message{});
