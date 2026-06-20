// RUN: -std=c++20

int parse_try_catch(int value) {
  try {
    value += 1;
  } catch (int error) {
    value = error;
  } catch (...) {
    value = 0;
  }
  return value;
}

void accepts_noexcept(void) noexcept {
}

void accepts_dynamic_exception_spec(void) throw(int) {
}

void side_effect(void) {
}

void conditional_void_arms(bool cond) {
  cond ? side_effect() : side_effect();
}

void accepts_noexcept_expression(void) noexcept(1) {
}

int lambda_noexcept_value(void) {
  return []() noexcept { return 7; }();
}

int throw_expression_statement(void) {
  throw 1;
}

int conditional_throw_right(bool cond) {
  return cond ? 7 : throw 2;
}

int conditional_throw_left(bool cond) {
  return cond ? throw 3 : 9;
}

void rethrow_in_catch(void) {
  try {
  } catch (...) {
    throw;
  }
}
