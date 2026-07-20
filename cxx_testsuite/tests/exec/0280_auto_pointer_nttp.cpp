int global_value = 17;

int add_one(int value) {
  return value + 1;
}

struct Record {
  int value;

  int add(int amount) {
    return value + amount;
  }
};

template <auto Value>
struct Holder {
  static int get() {
    return Value;
  }
};

template <auto Value>
struct NullHolder {
  static bool is_null() {
    return Value == (int*)nullptr;
  }
};

template <auto Value>
struct ObjectPointerHolder {
  static bool points_to(int* pointer) {
    return Value == pointer;
  }
};

template <auto Value>
struct FunctionPointerHolder {
  static int call(int argument) {
    return Value(argument);
  }
};

template <auto Value>
struct DataMemberPointerHolder {
  static int read(Record& record) {
    return record.*Value;
  }
};

template <auto Value>
struct FunctionMemberPointerHolder {
  static int call(Record& record, int argument) {
    return (record.*Value)(argument);
  }
};

template <typename TypeInfoT, auto FieldMember = nullptr,
          auto NamedMember = FieldMember, typename ReceiverFactoryT = void>
struct Metadata {
  static bool defaults_match() {
    return FieldMember == NamedMember;
  }
};

template <auto Value>
struct Category {
  static constexpr int value = 0;
};

template <int* Value>
struct Category<Value> {
  static constexpr int value = 1;
};

int main() {
  if (Holder<42>::get() != 42) {
    return 1;
  }
  if (!NullHolder<nullptr>::is_null()) {
    return 2;
  }
  if (!ObjectPointerHolder<&global_value>::points_to(&global_value)) {
    return 3;
  }
  if (FunctionPointerHolder<&add_one>::call(4) != 5) {
    return 4;
  }
  Record record{7};
  if (DataMemberPointerHolder<&Record::value>::read(record) != 7) {
    return 6;
  }
  if (FunctionMemberPointerHolder<&Record::add>::call(record, 3) != 10) {
    return 7;
  }
  Metadata<Record> default_metadata;
  (void)default_metadata;
  if (!Metadata<Record, &Record::value>::defaults_match()) {
    return 9;
  }
  if (Category<&global_value>::value != 1) {
    return 10;
  }
  return 0;
}
