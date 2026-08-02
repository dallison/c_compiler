// RUN: -std=c++23

struct move_only {
  move_only() = default;
  move_only(const move_only&) = delete;
  move_only(move_only&&) = default;
};

int main() {
  move_only value;
  (void)auto(value);
  return 0;
}
