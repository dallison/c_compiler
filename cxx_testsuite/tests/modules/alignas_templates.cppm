export module alignas_templates;

export template <typename... Ts>
struct alignas(Ts...) PackAligned {
  char c;
};

export template <unsigned long... Ns>
struct ValuePackMember {
  char c;
  alignas(4) alignas(Ns...) char m;
};

export template <typename T>
struct Outer {
  template <typename U>
  struct alignas(T) alignas(U) Inner {
    char c;
  };
};

export template <typename T>
bool LocalIsAligned() {
  alignas(T) alignas(2) char buffer[3];
  return (unsigned long)buffer % alignof(T) == 0;
}
