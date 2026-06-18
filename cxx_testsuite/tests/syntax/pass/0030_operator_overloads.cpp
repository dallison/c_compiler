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

struct ConvertibleNumber {
  int value;
  operator int() const {
    return value;
  }
  operator bool() const;
  operator int*();
  operator int&();
};

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
  ConvertibleNumber convertible;
  ArrowValue arrow_value;
  ArrowHolder arrow_holder;
  arrow_holder.ptr = &arrow_value;
  convertible.value = 1;
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
  int free_product = free_left * free_right;
  int free_less = free_left < free_right;
  int free_or = free_left | free_right;
  int free_not = !free_left;
  int free_comma = (free_left, free_right);
  int converted_int = convertible;
  int converted_bool = convertible ? 1 : 0;
  int* converted_ptr = convertible;
  int& converted_ref = convertible;
  int arrow_member = arrow_holder->value;
  return member_sum + member_diff + member_equal + member_shift + member_neg +
         member_deref + member_addr + member_ones + member_preinc +
         member_postinc + member_subscript + member_call + free_sum +
         member_comma + free_product + free_less + free_or + free_not +
         free_comma + converted_int + converted_bool + *converted_ptr +
         converted_ref + arrow_member;
}
