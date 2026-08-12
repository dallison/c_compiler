// RUN: -std=c++23 -O2
// EXPECT_EXIT: 0

static bool matches(const char* text, unsigned position, unsigned end) {
  return position + 1 < end && text[position + 1] == '<';
}

int main(int argc, char**) {
  unsigned end = static_cast<unsigned>(argc - 1);
  return matches(nullptr, 0, end) ? 1 : 0;
}
