extern "C" void foreign_throw_int();
extern "C" void foreign_throw_derived();
extern "C" void foreign_rethrow_davecc();

struct InteropBase {
  int value;
};

struct InteropDerived : InteropBase {};

extern "C" void davecc_throw_int() {
  throw 41;
}

extern "C" void davecc_throw_derived() {
  InteropDerived value;
  value.value = 43;
  throw value;
}

extern "C" int davecc_catch_foreign() {
  try {
    foreign_throw_int();
  } catch (int value) {
    return value == 42 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}

extern "C" int davecc_catch_foreign_derived() {
  try {
    foreign_throw_derived();
  } catch (const InteropBase& value) {
    return value.value == 44 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}

static int davecc_cleanup_count;

struct DaveCleanup {
  ~DaveCleanup() {
    davecc_cleanup_count++;
  }
};

extern "C" int davecc_cleanup_foreign() {
  davecc_cleanup_count = 0;
  try {
    DaveCleanup cleanup;
    foreign_throw_int();
  } catch (int value) {
    return value == 42 && davecc_cleanup_count == 1 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}

extern "C" int davecc_catch_foreign_rethrow() {
  try {
    foreign_rethrow_davecc();
  } catch (int value) {
    return value == 41 ? 0 : 2;
  } catch (...) {
    return 3;
  }
  return 1;
}
