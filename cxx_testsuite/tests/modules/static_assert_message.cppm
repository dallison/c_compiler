export module static_assert_message;

export struct module_message {
  char text[27];

  constexpr const char* data() const { return text; }
  constexpr unsigned size() const { return 26; }
};

export constexpr module_message generated{
    "module-generated assertion"};

export template <bool condition>
void verify_message() {
  static_assert(condition, generated);
}
