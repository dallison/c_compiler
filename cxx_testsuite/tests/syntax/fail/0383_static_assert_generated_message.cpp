// RUN: -std=c++26
// EXPECT: generated assertion text

struct message {
  char text[25];

  constexpr const char* data() const { return text; }
  constexpr unsigned size() const { return 24; }
};

constexpr message generated{"generated assertion text"};

static_assert(false, generated);
