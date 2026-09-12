#ifndef value_state_h
#define value_state_h

#ifndef COMPILER_UNUSED
#if defined(__GNUC__) || defined(__clang__)
#define COMPILER_UNUSED __attribute__((unused))
#else
#define COMPILER_UNUSED
#endif
#endif

// C++26 [basic.indet] value state.  Object lifetime/activity is tracked
// independently; a live object's bytes can have any of these states.
typedef enum ValueState {
  kValueStateValid = 0,
  kValueStateErroneous = 1,
  kValueStateIndeterminate = 2,
} ValueState;

static COMPILER_UNUSED ValueState ValueStateMerge(ValueState left,
                                                  ValueState right) {
  if (left == right) {
    return left;
  }
  if (left == kValueStateIndeterminate ||
      right == kValueStateIndeterminate) {
    return kValueStateIndeterminate;
  }
  return kValueStateErroneous;
}

#endif /* value_state_h */
