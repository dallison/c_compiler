// RUN: -std=c++17
struct MemberNumber {
  int value;
  int operator+(int value);
  int operator-(int value);
  int operator-(void);
  int operator*(void);
  int* operator&(void);
  int operator~(void);
  int operator++(void);
  int operator++(int value);
  int operator[](int value);
  int operator()(int value);
  int operator,(int value);
  int operator->*(int value);
  int operator==(int value);
  int operator<<(int value);
};

int MemberNumber::operator+(int value) {
  return value;
}

int MemberNumber::operator-(int value) {
  return value;
}

int MemberNumber::operator-(void) {
  return 6;
}

int MemberNumber::operator*(void) {
  return 9;
}

int* MemberNumber::operator&(void) {
  return 0;
}

int MemberNumber::operator~(void) {
  return 7;
}

int MemberNumber::operator++(void) {
  return 10;
}

int MemberNumber::operator++(int value) {
  return 11 + value;
}

int MemberNumber::operator[](int value) {
  return value + 12;
}

int MemberNumber::operator()(int value) {
  return value + 13;
}

int MemberNumber::operator,(int value) {
  return value + 14;
}

int MemberNumber::operator->*(int value) {
  return value + 15;
}

int MemberNumber::operator==(int value) {
  return value;
}

int MemberNumber::operator<<(int value) {
  return value;
}

struct FreeNumber {
  int value;
};

struct SfinaeOperatorNumber {
  int value;
};

struct SfinaeDependentOperatorNumber {
  int value;
};

struct HasNestedOperatorType {
  int value;
  struct type {
    int value;
  };
};

struct ConvertibleNumber {
  int value;
  operator int() const {
    return value;
  }
  operator bool() const;
  operator int*();
  operator int&();
};

struct ExplicitNumber {
  int value;
  explicit operator int() const {
    return value + 1;
  }
  explicit operator bool() const;
};

ExplicitNumber::operator bool() const {
  return value != 0;
}

ConvertibleNumber::operator bool() const {
  return value != 0;
}

ConvertibleNumber::operator int*() {
  return &value;
}

ConvertibleNumber::operator int&() {
  return value;
}

struct ArrowValue {
  int value;
};

struct ArrowHolder {
  ArrowValue* ptr;
  ArrowValue* operator->(void);
};

ArrowValue* ArrowHolder::operator->(void) {
  return ptr;
}

int operator+(FreeNumber left, FreeNumber right) {
  return 2;
}

template <typename T, typename U>
U operator+(SfinaeOperatorNumber left, T right) {
  return right;
}

int operator+(SfinaeOperatorNumber left, char right) {
  return left.value + right + 30;
}

template <typename T>
typename T::type operator/(SfinaeDependentOperatorNumber left, T right) {
  typename T::type result;
  result.value = left.value + right.value + 70;
  return result;
}

int operator/(SfinaeDependentOperatorNumber left, int right) {
  return left.value + right + 80;
}

template <typename T>
typename T::type operator%(SfinaeDependentOperatorNumber left, T right) {
  typename T::type result;
  result.value = left.value + right.value + 90;
  return result;
}

int operator*(FreeNumber left, FreeNumber right) {
  return 3;
}

int operator<(FreeNumber left, FreeNumber right) {
  return 4;
}

int operator|(FreeNumber left, FreeNumber right) {
  return 5;
}

int operator!(FreeNumber value) {
  return 8;
}

int operator,(FreeNumber left, FreeNumber right) {
  return 14;
}

int main(void) {
  MemberNumber member_left;
  MemberNumber member_right;
  FreeNumber free_left;
  FreeNumber free_right;
  SfinaeOperatorNumber sfinae_operator;
  SfinaeDependentOperatorNumber sfinae_dependent_operator;
  HasNestedOperatorType nested_operator_type;
  ConvertibleNumber convertible;
  ExplicitNumber explicit_number;
  ArrowValue arrow_value;
  ArrowHolder arrow_holder;
  arrow_holder.ptr = &arrow_value;
  sfinae_dependent_operator.value = 3;
  nested_operator_type.value = 4;
  convertible.value = 1;
  explicit_number.value = 1;
  int member_sum = member_left + 1;
  int member_diff = member_left - 1;
  int member_equal = member_left == 1;
  int member_shift = member_left << 1;
  int member_neg = -member_left;
  int member_deref = *member_left;
  int member_addr = &member_left == 0;
  int member_ones = ~member_left;
  int member_preinc = ++member_left;
  int member_postinc = member_left++;
  int member_subscript = member_left[1];
  int member_call = member_left(1);
  int member_comma = (member_left, 1);
  int free_sum = free_left + free_right;
  int sfinae_operator_sum = sfinae_operator + 'a';
  int sfinae_dependent_operator_fallback = sfinae_dependent_operator / 5;
  typename HasNestedOperatorType::type sfinae_dependent_operator_template =
      sfinae_dependent_operator % nested_operator_type;
  int free_product = free_left * free_right;
  int free_less = free_left < free_right;
  int free_or = free_left | free_right;
  int free_not = !free_left;
  int free_comma = (free_left, free_right);
  int converted_int = convertible;
  int converted_bool = convertible ? 1 : 0;
  int* converted_ptr = convertible;
  int& converted_ref = convertible;
  int explicit_int = static_cast<int>(explicit_number);
  int explicit_bool = explicit_number ? 1 : 0;
  int arrow_member = arrow_holder->value;
  return member_sum + member_diff + member_equal + member_shift + member_neg +
         member_deref + member_addr + member_ones + member_preinc +
         member_postinc + member_subscript + member_call + free_sum +
         sfinae_operator_sum + member_comma + free_product + free_less +
         sfinae_dependent_operator_fallback +
         sfinae_dependent_operator_template.value + free_or + free_not +
         free_comma + converted_int + converted_bool + *converted_ptr +
         converted_ref + explicit_int + explicit_bool +
         arrow_member;
}
