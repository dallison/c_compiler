struct MemberNumber {
  int value;
  int operator*(void);
  int* operator&(void);
  int operator++(void);
  int operator++(int value);
  int operator[](int value);
  int operator()(int value);
  int operator,(int value);
};

int MemberNumber::operator*(void) {
  return value + 1;
}

int* MemberNumber::operator&(void) {
  return 0;
}

int MemberNumber::operator++(void) {
  value += 1;
  return value;
}

int MemberNumber::operator++(int ignored) {
  int old = value;
  value += 1 + ignored;
  return old;
}

int MemberNumber::operator[](int index) {
  return value + index;
}

int MemberNumber::operator()(int arg) {
  return value + arg;
}

int MemberNumber::operator,(int arg) {
  return value + arg + 10;
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

int operator,(FreeNumber left, FreeNumber right) {
  return left.value + right.value + 20;
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

struct Item {
  int x;
  int y;
};

struct Iter {
  Item* ptr;
  struct Item operator*(void);
  struct Iter operator++(void);
  int operator!=(Iter other);
};

struct Item Iter::operator*(void) {
  return *ptr;
}

struct Iter Iter::operator++(void) {
  ptr += 1;
  return *this;
}

int Iter::operator!=(Iter other) {
  return ptr != other.ptr;
}

struct Range {
  Item* first;
  Item* last;
  Iter begin();
  Iter end();
};

Iter Range::begin() {
  Iter it;
  it.ptr = first;
  return it;
}

Iter Range::end() {
  Iter it;
  it.ptr = last;
  return it;
}

struct ArrowHolder {
  Item* ptr;
  Item* operator->(void);
};

Item* ArrowHolder::operator->(void) {
  return ptr;
}

int main(void) {
  MemberNumber n;
  n.value = 5;
  if (*n != 6) {
    return 1;
  }
  if (&n != 0) {
    return 2;
  }
  if (++n != 6 || n.value != 6) {
    return 3;
  }
  if (n++ != 6 || n.value != 7) {
    return 4;
  }
  if (n[2] != 9) {
    return 5;
  }
  if (n(3) != 10) {
    return 6;
  }
  if ((n, 4) != 21) {
    return 9;
  }

  FreeNumber free_left;
  FreeNumber free_right;
  free_left.value = 1;
  free_right.value = 2;
  if ((free_left, free_right) != 23) {
    return 10;
  }
  SfinaeOperatorNumber sfinae_operator;
  sfinae_operator.value = 2;
  if (sfinae_operator + 'a' != 129) {
    return 19;
  }
  SfinaeDependentOperatorNumber sfinae_dependent_operator;
  sfinae_dependent_operator.value = 3;
  if (sfinae_dependent_operator / 5 != 88) {
    return 20;
  }
  HasNestedOperatorType nested_operator_type;
  nested_operator_type.value = 4;
  typename HasNestedOperatorType::type sfinae_dependent_operator_template =
      sfinae_dependent_operator % nested_operator_type;
  if (sfinae_dependent_operator_template.value != 97) {
    return 21;
  }
  ConvertibleNumber convertible;
  convertible.value = 9;
  int converted_int = convertible;
  if (converted_int != 9) {
    return 11;
  }
  if (!convertible) {
    return 12;
  }
  int* converted_ptr = convertible;
  if (converted_ptr != &convertible.value || *converted_ptr != 9) {
    return 14;
  }
  int& converted_ref = convertible;
  converted_ref = 4;
  if (convertible.value != 4) {
    return 15;
  }
  convertible.value = 0;
  if (convertible) {
    return 13;
  }
  ExplicitNumber explicit_number;
  explicit_number.value = 6;
  int explicit_int = static_cast<int>(explicit_number);
  if (explicit_int != 7) {
    return 16;
  }
  if (!explicit_number) {
    return 17;
  }
  explicit_number.value = 0;
  if (explicit_number) {
    return 18;
  }

  Item items[2];
  items[0].x = 1;
  items[0].y = 2;
  items[1].x = 3;
  items[1].y = 4;
  Range range;
  range.first = items;
  range.last = items + 2;
  int sum = 0;
  for ([x, y] : range) {
    sum += x * 10 + y;
  }
  if (sum != 46) {
    return 7;
  }
  ArrowHolder holder;
  holder.ptr = items;
  if (holder->y != 2) {
    return 8;
  }
  return 0;
}
