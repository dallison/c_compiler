// RUN: -std=c++26
// EXPECT-ASM: _Z8identityIXtl7storageu127dave_designated_object_4d303b746c4134
// EXPECT-ASM: _Z18duplicate_identityIXtl17duplicate_storageu57dave_designated_object_4d303b
// EXPECT-ASM: _Z18duplicate_identityIXtl17duplicate_storageu57dave_designated_object_4d313b

union storage {
  int elements[4];
  int fallback;
};

constexpr storage make_storage() {
  storage result;
  __builtin_start_lifetime(&result.elements);
  result.elements[3] = 42;
  return result;
}

struct value {
  int number;
};

union duplicate_storage {
  value first;
  value second;
};

constexpr duplicate_storage make_first_storage() {
  duplicate_storage result;
  __builtin_start_lifetime(&result.first);
  result.first = value{42};
  return result;
}

constexpr duplicate_storage make_second_storage() {
  duplicate_storage result;
  __builtin_start_lifetime(&result.second);
  result.second = value{42};
  return result;
}

template <storage Value>
int identity() {
  return Value.elements[3];
}

template <duplicate_storage Value>
int duplicate_identity() {
  return 0;
}

int use() {
  return identity<make_storage()>() +
         duplicate_identity<make_first_storage()>() +
         duplicate_identity<make_second_storage()>();
}
