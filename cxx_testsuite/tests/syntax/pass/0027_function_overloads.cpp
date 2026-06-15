// RUN: -std=c++17
int pick(int value) {
  return value;
}

int pick(char value) {
  return value;
}

int pick(int *value) {
  return sizeof(value);
}

int arity(int value) {
  return value;
}

int arity(int left, int right) {
  return left + right;
}

int *returns_by_arg(int *value) {
  return value;
}

char *returns_by_arg(char *value) {
  return value;
}

int *returns_by_arg(int value) {
  return nullptr;
}

namespace ns {
int namespaced(int value) {
  return value;
}

char namespaced(char value) {
  return value;
}
}

int main(void) {
  char c;
  int i;
  char *cp;
  int *p;
  char *selected_char_pointer = returns_by_arg(cp);
  int *selected_int_pointer = returns_by_arg(p);
  int *selected_int_value = returns_by_arg(i);
  int selected_namespace_int = ns::namespaced(i);
  char selected_namespace_char = ns::namespaced(c);
  return sizeof(pick(c)) + sizeof(pick(i)) + sizeof(pick(p)) +
         sizeof(pick(nullptr)) + sizeof(arity(1)) + sizeof(arity(1, 2)) +
         sizeof(selected_char_pointer) + sizeof(selected_int_pointer) +
         sizeof(selected_int_value) + sizeof(selected_namespace_int) +
         sizeof(selected_namespace_char);
}
