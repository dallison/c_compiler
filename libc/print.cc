#include <print>

namespace std {

namespace __print_detail {

void __write_stdout(const string& value) {
  size_t position = 0;
  while (position < value.size()) {
    int written = ::write(1, value.data() + position, value.size() - position);
    if (written <= 0) {
      __format_detail::__fail("failed to write formatted output");
    }
    position += static_cast<size_t>(written);
  }
}

void __write_file(FILE* file, const string& value) {
  if (file == nullptr ||
      ::fwrite(value.data(), 1, value.size(), file) != value.size()) {
    __format_detail::__fail("failed to write formatted output");
  }
}

}  // namespace __print_detail

void vprint_nonunicode(string_view text, format_args args) {
  __print_detail::__write_stdout(vformat(text, args));
}

void vprint_nonunicode(FILE* file, string_view text, format_args args) {
  __print_detail::__write_file(file, vformat(text, args));
}

void vprint_unicode(string_view text, format_args args) {
  __print_detail::__write_stdout(vformat(text, args));
}

void vprint_unicode(FILE* file, string_view text, format_args args) {
  __print_detail::__write_file(file, vformat(text, args));
}

void print(
    const typename __format_detail::__non_deduced<format_string<>>::type& text) {
  __print_detail::__write_stdout(
      __format_detail::__format_invoke(text.data(), text.size()));
}

void print(
    FILE* file,
    const typename __format_detail::__non_deduced<format_string<>>::type& text) {
  __print_detail::__write_file(
      file, __format_detail::__format_invoke(text.data(), text.size()));
}

void println(
    const typename __format_detail::__non_deduced<format_string<>>::type& text) {
  string output =
      __format_detail::__format_invoke(text.data(), text.size());
  output.push_back('\n');
  __print_detail::__write_stdout(output);
}

void println() {
  __print_detail::__write_stdout(string("\n"));
}

void println(
    FILE* file,
    const typename __format_detail::__non_deduced<format_string<>>::type& text) {
  string output =
      __format_detail::__format_invoke(text.data(), text.size());
  output.push_back('\n');
  __print_detail::__write_file(file, output);
}

void println(FILE* file) {
  if (file == nullptr || ::fwrite("\n", 1, 1, file) != 1) {
    __format_detail::__fail("print write failed");
  }
}

}  // namespace std
