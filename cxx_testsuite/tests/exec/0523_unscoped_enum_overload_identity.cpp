// RUN: -std=c++20
// EXPECT_EXIT: 0

enum syntax_option_type {
  icase = 1 << 0,
  nosubs = 1 << 1,
};

enum match_flag_type {
  match_default = 0,
  match_not_bol = 1 << 0,
};

syntax_option_type operator|(syntax_option_type left, syntax_option_type right) {
  return static_cast<syntax_option_type>(
      static_cast<int>(left) | static_cast<int>(right));
}

match_flag_type operator|(match_flag_type left, match_flag_type right) {
  return static_cast<match_flag_type>(
      static_cast<int>(left) | static_cast<int>(right));
}

syntax_option_type syntax_or(syntax_option_type left, syntax_option_type right) {
  return left | right;
}

match_flag_type match_or(match_flag_type left, match_flag_type right) {
  return left | right;
}

int main() {
  if (static_cast<int>(syntax_or(icase, nosubs)) != (icase | nosubs)) {
    return 1;
  }
  if (static_cast<int>(match_or(match_default, match_not_bol)) !=
      match_not_bol) {
    return 2;
  }
  return 0;
}
