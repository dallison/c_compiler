// In a generic lambda outside any template, `decltype(value.member)` names the
// member's declared type once `value`'s type is deduced, not the type of
// `value` itself, and a template-id naming it stays dependent until then.
template <class T> struct remove_pointer { using type = T; };
template <class T> struct remove_pointer<T*> { using type = T; };

struct Big {
  char bytes[24];
};
struct Holder {
  const Big* ptr;
  short small;
};

int main() {
  static const Big big = {{7}};
  const Holder holder{&big, 3};
  int failures = 0;
  auto inspect = [&failures](const auto& value) {
    using Ptr = decltype(value.ptr);
    using Pointee = typename remove_pointer<Ptr>::type;
    using Small = decltype(value.small);
    Ptr copy = value.ptr;
    if (sizeof(Pointee) != 24 || copy->bytes[0] != 7) failures |= 1;
    if (sizeof(Small) != sizeof(short)) failures |= 2;
    Small doubled = static_cast<Small>(value.small * 2);
    if (doubled != 6) failures |= 4;
    return sizeof(Ptr);
  };
  if (inspect(holder) != sizeof(void*)) failures |= 8;
  return failures;
}
