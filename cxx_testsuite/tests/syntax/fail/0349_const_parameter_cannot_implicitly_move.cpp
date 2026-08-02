// RUN: -std=c++23
// EXPECT: No viable constructor for return value

struct move_only {
  move_only() = default;
  move_only(const move_only&) = delete;
  move_only(move_only&&) = default;
};

move_only function(const move_only value) {
  return value;
}
