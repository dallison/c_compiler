// RUN: -std=c++20 -fno-exceptions -fexceptions

// A later -fexceptions re-enables exception handling that an earlier
// -fno-exceptions disabled (last-one-wins), so throw/try are accepted again.
int uses_exceptions(int value) {
  try {
    throw value;
  } catch (int caught) {
    value = caught;
  }
  return value;
}
