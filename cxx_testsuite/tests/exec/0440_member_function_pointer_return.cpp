// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// A pointer to member function is a pair of words, too wide for a result
// register, so a function returning one has to fill the caller's hidden result
// buffer the way a class return does.  Returning one used to leave the caller's
// destination untouched and put the source address in the result register, so
// the caller called through uninitialized stack.

struct receiver {
  int value = 0;
  void add(int increment) { value += increment; }
  int scaled(int factor) const { return value * factor; }
};

using Adder = void (receiver::*)(int);
using Scaler = int (receiver::*)(int) const;
using Field = int receiver::*;

__attribute__((noinline)) static Adder FromReference(Adder& source) {
  return source;
}

__attribute__((noinline)) static Adder FromValue(Adder source) {
  return source;
}

static Adder Inlined(Adder& source) { return source; }

__attribute__((noinline)) static Scaler ConstQualified() {
  return &receiver::scaled;
}

__attribute__((noinline)) static Field DataMember() {
  return &receiver::value;
}

struct factory {
  Adder held;

  __attribute__((noinline)) Adder get() const { return held; }
};

// A reference parameter has to bind to a materialized copy of the returned
// prvalue, not to the callee's source.
struct holder {
  Adder function;
  explicit holder(Adder&& source) : function(static_cast<Adder&&>(source)) {}
};

int main() {
  Adder adder = &receiver::add;

  receiver first;
  (first.*FromReference(adder))(7);
  if (first.value != 7) {
    return 1;
  }

  receiver second;
  (second.*FromValue(adder))(9);
  if (second.value != 9) {
    return 2;
  }

  receiver third;
  Adder copy = Inlined(adder);
  (third.*copy)(11);
  if (third.value != 11) {
    return 3;
  }

  receiver fourth;
  holder bound(Inlined(adder));
  (fourth.*bound.function)(13);
  if (fourth.value != 13) {
    return 4;
  }

  factory source{adder};
  receiver fifth;
  (fifth.*source.get())(15);
  if (fifth.value != 15) {
    return 5;
  }

  receiver sixth;
  sixth.value = 6;
  if ((sixth.*ConstQualified())(4) != 24) {
    return 6;
  }

  // A pointer to a data member is one word, so it keeps coming back in the
  // result register; it is here to pin that down.
  Field field = DataMember();
  if (sixth.*field != 6) {
    return 7;
  }

  return 0;
}
