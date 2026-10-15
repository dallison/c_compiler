// A lambda nested in a lambda of a function template (protobuf's
// MicroString::SetInChunks) whose body calls a function template
// (`GetReferenceableValue` inside ABSL_DCHECK_LE) is handed to a generic
// lambda that calls it.  The call's callee is typed with the function
// template's own signature; that must not read as the inner lambda needing
// enclosing template arguments it does not have, or its operator() is never
// defined.
#include <string.h>
#include <string_view>

template <typename T>
const T& GetReferenceableValue(const T& t) {
  return t;
}

int total = 0;

struct MicroString {
  char buf[16];
  long len = 0;

  template <typename F>
  void SetInChunks(long size, F setter) {
    const auto invoke_setter = [&](char* p) {
      char* start = p;
      setter([&](std::string_view chunk) {
        if (GetReferenceableValue(p - start + (long)chunk.size()) > size) {
          __builtin_trap();
        }
        memcpy(p, chunk.data(), chunk.size());
        p += chunk.size();
      });
      return p - start;
    };
    len = invoke_setter(buf);
  }
};

template <typename F>
void Twice(F setter) {
  const auto outer = [&](int base) {
    setter([&](int c) { total += GetReferenceableValue(c) + base; });
  };
  outer(10);
  outer(20);
}

int main() {
  MicroString s;
  const char* text = "hello world";
  s.SetInChunks(11, [&](auto append) {
    append(std::string_view(text, 5));
    append(std::string_view(text + 5, 6));
  });
  if (s.len != 11 || memcmp(s.buf, text, 11) != 0) {
    return 1;
  }
  Twice([&](auto append) { append(1); });
  return total == 32 ? 0 : 2;
}
