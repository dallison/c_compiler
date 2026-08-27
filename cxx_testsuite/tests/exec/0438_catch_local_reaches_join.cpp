// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

volatile int sink;

__attribute__((noinline)) static void Thrower(int v) {
  throw v;
}

// A landing pad is entered without any branch reaching it, so the optimizer used
// to see it as having no predecessor: it stayed out of the dominator tree, the
// store to `caught` never got an SSA name, and the read after the try still saw
// the pre-try value.
__attribute__((noinline)) static int CaughtLocal() {
  int caught = 0;
  try {
    Thrower(9);
  } catch (int value) {
    caught = value;
  }
  return caught;
}

// The same shape where the handler leaves the local alone: the value defined
// before the try has to survive the join too.
__attribute__((noinline)) static int UntouchedLocal(bool throwing) {
  int value = 7;
  try {
    if (throwing) {
      Thrower(1);
    }
  } catch (int) {
  }
  return value;
}

// Both paths into the join define the local, so neither the pre-try value nor a
// stale handler value may leak through.
__attribute__((noinline)) static int BothPathsDefine(bool throwing) {
  int value = 0;
  try {
    if (throwing) {
      Thrower(2);
    }
    value = 3;
  } catch (int caught) {
    value = caught + 40;
  }
  return value;
}

// A handler that reads the local it was going to overwrite needs the definition
// from before the try to reach it.
__attribute__((noinline)) static int HandlerReadsPreTryValue() {
  int value = 5;
  try {
    Thrower(1);
  } catch (int caught) {
    value += caught;
  }
  return value;
}

int main() {
  if (CaughtLocal() != 9) {
    return 1;
  }
  if (UntouchedLocal(false) != 7) {
    return 2;
  }
  if (UntouchedLocal(true) != 7) {
    return 3;
  }
  if (BothPathsDefine(false) != 3) {
    return 4;
  }
  if (BothPathsDefine(true) != 42) {
    return 5;
  }
  if (HandlerReadsPreTryValue() != 6) {
    return 6;
  }
  return 0;
}
