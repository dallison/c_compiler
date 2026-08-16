// RUN: -std=c++26

#include <meta>

using namespace std::meta;

static_assert(!is_aggregate_type(^^access_context));

class Base {
  int secret;

 protected:
  int protected_member;

 public:
  int public_member;

  static consteval info secret_ref() { return ^^Base::secret; }
  static consteval info protected_ref() { return ^^Base::protected_member; }
  static consteval info public_ref() { return ^^Base::public_member; }

  static consteval bool local_access_checks() {
    return access_context::current().scope() == current_function() &&
           is_accessible(secret_ref(), access_context::current()) &&
           !is_accessible(secret_ref(), access_context::unprivileged());
  }
};

class Derived : public Base {
 public:
  static consteval bool access_checks() {
    return !is_accessible(secret_ref(), access_context::current()) &&
           !is_accessible(protected_ref(), access_context::current()) &&
           is_accessible(protected_ref(),
                         access_context::current().via(^^Derived)) &&
           is_accessible(public_ref(), access_context::current());
  }
};

struct Unrelated {};
struct Nested {
  int value;
};
struct Aggregate : Base {
  Nested nested;
};
struct Empty {};

consteval bool exception_checks() {
  try {
    (void)parent_of(^^int);
  } catch (const exception& error) {
    return error.where().line() == 51;
  }
  return false;
}

static_assert(Base::local_access_checks());
static_assert(Derived::access_checks());
static_assert(!is_accessible(Base::secret_ref(),
                             access_context::unprivileged()));
static_assert(is_accessible(Base::secret_ref(), access_context::unchecked()));
static_assert(!is_accessible(
    Base::public_ref(), access_context::unchecked().via(^^Unrelated)));

static_assert(
    subobjects_of(^^Aggregate, access_context::unchecked()).size() == 2);
static_assert(
    members_of(^^Empty, access_context::unchecked()).size() == 6);
static_assert(exception_checks());
