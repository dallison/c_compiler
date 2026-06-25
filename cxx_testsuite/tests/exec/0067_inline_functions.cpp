// RUN: -std=c++20

inline int inline_before_use(int value) {
  return value + 1;
}

int declared_before_inline_definition(int value);

inline int declared_before_inline_definition(int value) {
  return value + 2;
}

inline int inline_declaration_before_definition(int value);

int inline_declaration_before_definition(int value) {
  return value + 3;
}

inline int inline_definition_before_declaration(int value) {
  return value + 4;
}

int inline_definition_before_declaration(int value);

namespace inline_ns {
inline int namespaced_inline(int value) {
  return value + 5;
}
}

struct InlineMemberBox {
  int value;
  int member_inline(void) {
    return value + 6;
  }
};

int main(void) {
  InlineMemberBox box;
  box.value = 10;
  int result = inline_before_use(1) +
               declared_before_inline_definition(2) +
               inline_declaration_before_definition(3) +
               inline_definition_before_declaration(4) +
               inline_ns::namespaced_inline(5) +
               box.member_inline();
  return result == 46 ? 0 : result;
}
