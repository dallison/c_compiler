// `field->CPPTYPE_ENUM` names an enumerator of a base class; inside a function
// template the cloned access is analyzed before it is reattached, and it must
// still fold to the constant rather than reference a nonexistent data member.
struct Base0 {
  int pad;
};

struct Lite {
  enum CppType { CPPTYPE_INT32 = 1, CPPTYPE_ENUM = 8 };
  CppType t;
};

struct Field : private Base0, public Lite {};

template <typename T>
int Normalize(const Field* field, Lite::CppType cpp_type) {
  if (cpp_type == field->CPPTYPE_ENUM) cpp_type = field->CPPTYPE_INT32;
  return cpp_type;
}

template <typename T>
int Direct(const Field& field) {
  return field.CPPTYPE_ENUM;
}

int NonTemplate(const Field* field) { return field->CPPTYPE_ENUM; }

int main() {
  Field f;
  if (Normalize<int>(&f, Lite::CPPTYPE_ENUM) != 1) return 1;
  if (Normalize<char>(&f, Lite::CPPTYPE_INT32) != 1) return 2;
  if (Direct<int>(f) != 8) return 3;
  if (NonTemplate(&f) != 8) return 4;
  return 0;
}
