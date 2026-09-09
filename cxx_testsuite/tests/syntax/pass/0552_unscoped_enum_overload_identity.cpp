// RUN: -std=c++20

enum syntax_option_type {
  icase = 1,
  nosubs = 2,
};

enum match_flag_type {
  match_default = 0,
  match_not_bol = 1,
};

syntax_option_type operator|(syntax_option_type left, syntax_option_type right) {
  return static_cast<syntax_option_type>(
      static_cast<int>(left) | static_cast<int>(right));
}

match_flag_type operator|(match_flag_type left, match_flag_type right) {
  return static_cast<match_flag_type>(
      static_cast<int>(left) | static_cast<int>(right));
}

int main() {
  syntax_option_type options = icase | nosubs;
  match_flag_type flags = match_default | match_not_bol;
  return static_cast<int>(options) + static_cast<int>(flags);
}
