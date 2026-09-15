// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <string_view>

// A consteval constructor that stores a pointer into a string-literal
// argument must materialize a guest address, not the constexpr p-code
// interpreter's host pointer.  `std::format_string` depends on this.
//
// `p ? ptr_member : ""` must also be a prvalue pointer.  If it stays an
// lvalue, `std::string_view(that, n)` receives the address of the member
// instead of the stored pointer, which broke chrono/filesystem formatters.

struct literal_view {
  const char* data;
  int size;

  template <unsigned N>
  consteval literal_view(const char (&text)[N]) : data(text), size(N - 1) {
    unsigned sum = 0;
    for (unsigned i = 0; i < N; ++i) {
      sum += static_cast<unsigned char>(text[i]);
    }
    (void)sum;
  }
};

static int check_view(const literal_view& view, const char* expected,
                      int size) {
  if (view.size != size) {
    return 1;
  }
  if (view.data == nullptr) {
    return 2;
  }
  for (int i = 0; i <= size; ++i) {
    if (view.data[i] != expected[i]) {
      return 10 + i;
    }
  }
  return 0;
}

int main() {
  literal_view view("abcdef");
  int result = check_view(view, "abcdef", 6);
  if (result != 0) {
    return result;
  }

  result = check_view("%F %T %Z %z", "%F %T %Z %z", 11);
  if (result != 0) {
    return 20 + result;
  }

  const char* lit = "abcdef";
  struct spec {
    const char* data;
    int size;
  } value = {lit, 6};
  const spec* pointer = &value;
  std::string_view text(pointer != nullptr && pointer->data != nullptr
                            ? pointer->data
                            : "",
                        pointer != nullptr ? static_cast<size_t>(pointer->size)
                                           : 0);
  if (text.data() != lit || text.size() != 6) {
    return 30;
  }
  return 0;
}
