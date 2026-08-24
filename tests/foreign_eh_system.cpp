extern "C" void davecc_throw_int();
extern "C" void davecc_throw_derived();
extern "C" int davecc_catch_foreign();
extern "C" int davecc_catch_foreign_derived();
extern "C" int davecc_cleanup_foreign();
extern "C" int davecc_catch_foreign_rethrow();

struct InteropBase {
  int value;
};

struct InteropDerived : InteropBase {};

volatile int foreign_eh_stage = -1;

extern "C" void foreign_throw_int() {
  throw 42;
}

extern "C" void foreign_throw_derived() {
  InteropDerived value;
  value.value = 44;
  throw value;
}

extern "C" void foreign_rethrow_davecc() {
  try {
    davecc_throw_int();
  } catch (int) {
    throw;
  }
}

static int foreign_cleanup_count;

struct ForeignCleanup {
  ~ForeignCleanup() {
    foreign_cleanup_count++;
  }
};

static int foreign_catch_davecc_derived() {
  try {
    davecc_throw_derived();
  } catch (const InteropBase& value) {
    return value.value == 43 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}

static int foreign_cleanup_davecc() {
  foreign_cleanup_count = 0;
  try {
    ForeignCleanup cleanup;
    davecc_throw_int();
  } catch (int value) {
    return value == 41 && foreign_cleanup_count == 1 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}

int main() {
  foreign_eh_stage = 1;
  try {
    davecc_throw_int();
  } catch (int value) {
    if (value != 41) {
      return 1;
    }
  } catch (...) {
    return 2;
  }

  foreign_eh_stage = 2;
  if (davecc_catch_foreign() != 0) {
    return 3;
  }
  foreign_eh_stage = 3;
  if (foreign_catch_davecc_derived() != 0) {
    return 4;
  }
  foreign_eh_stage = 4;
  if (davecc_catch_foreign_derived() != 0) {
    return 5;
  }
  foreign_eh_stage = 5;
  if (foreign_cleanup_davecc() != 0) {
    return 6;
  }
  foreign_eh_stage = 6;
  if (davecc_cleanup_foreign() != 0) {
    return 7;
  }
  foreign_eh_stage = 7;
  return davecc_catch_foreign_rethrow() == 0 ? 0 : 8;
}
