#include "test_framework.h"

#include <exception>

struct Pod {
  int value;
};

struct Base {
  virtual ~Base() {}
};

struct Derived : Base {
  int value;
  Derived(int v) : value(v) {}
};

static int cleanup_runs;

struct CleanupProbe {
  ~CleanupProbe() { cleanup_runs++; }
};

static int catch_typed_int(void) {
  try {
    throw 42;
  } catch (int value) {
    return value == 42 ? 0 : 1;
  }
}

static int catch_all_probe(void) {
  try {
    throw 7;
  } catch (...) {
    return 0;
  }
}

static int ordered_handlers(void) {
  try {
    throw 23;
  } catch (char) {
    return 1;
  } catch (int value) {
    return value == 23 ? 0 : 2;
  } catch (...) {
    return 3;
  }
}

static int catch_derived_as_base(void) {
  try {
    throw Derived(99);
  } catch (const Base&) {
    return 0;
  }
}

static int cleanup_before_catch(void) {
  cleanup_runs = 0;
  try {
    CleanupProbe probe;
    throw 1;
  } catch (int) {
    return cleanup_runs == 1 ? 0 : 2;
  }
}

static int rethrow_and_catch(void) {
  try {
    try {
      throw 5;
    } catch (int) {
      throw;
    }
  } catch (int value) {
    return value == 5 ? 0 : 3;
  }
}

static int catch_pod(void) {
  try {
    throw Pod{17};
  } catch (const Pod& caught) {
    return caught.value == 17 ? 0 : 4;
  }
}

extern "C" int TestEHItaniumRuntime(void) {
  if (catch_typed_int() != 0) return 1;
  if (catch_all_probe() != 0) return 2;
  if (ordered_handlers() != 0) return 3;
  if (catch_derived_as_base() != 0) return 4;
  if (cleanup_before_catch() != 0) return 5;
  if (rethrow_and_catch() != 0) return 6;
  if (catch_pod() != 0) return 7;
  return 0;
}
