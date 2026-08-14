// RUN: -std=c++26 -fconstexpr-eval=audit

struct audit_error {
  int value;
};

constexpr void audit_throw(bool class_type) {
  if (class_type) {
    throw audit_error{29};
  }
  throw 11;
}

constexpr int audit_dispatch(bool class_type) {
  try {
    audit_throw(class_type);
  } catch (int value) {
    return value;
  } catch (const audit_error& value) {
    return value.value;
  }
}

static_assert(audit_dispatch(false) == 11);
static_assert(audit_dispatch(true) == 29);
