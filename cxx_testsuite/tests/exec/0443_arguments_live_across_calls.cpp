// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// Two RISC-V register allocator defects, both of which need a function whose
// register variables have taken every callee-saved register the allocator hands
// out freely, and several unnamed values that have to survive a call.  That is
// how `std::thread`'s constructor is shaped: `new __state(__decay_copy(f),
// __decay_copy(a)...)` materializes each decayed argument into a local, and the
// addresses of those locals stay live across the remaining __decay_copy calls
// because the state constructor takes them by reference.  `holder` below is that
// constructor reduced to its shape, and the chain of `decay` calls occupies the
// callee-saved registers the way the surrounding std::thread code does.
//
// A value that has to survive a call needs a callee-saved register, and when
// none was free the allocator evicted whichever value was cheapest to spill --
// including one sitting in a caller-saved temp -- and then took that temp for
// itself, so the call it had to live across clobbered it.
//
// A call's arguments are placed by a parallel copy, which has to be ordered so
// that no move overwrites a register a later one still reads.  When one of those
// moves loaded its argument from a spill slot it became a reload, which is not a
// register-to-register move at all, and the resolver read it as a malformed
// member of the copy and gave up on the whole run -- leaving the moves in their
// original order, where `mv a1,t1` preceded the `mv a0,a1` that still needed the
// old a1.

struct receiver {
  int value = 0;

  void add(int increment) { value += increment; }
};

using member_type = void (receiver::*)(int);

template <class Value>
static Value decay_copy(Value&& value) {
  return static_cast<Value&&>(value);
}

struct holder {
  member_type member;
  receiver* object;
  int amount;

  holder(member_type&& a_member, receiver*&& a_object, int&& a_amount)
      : member(a_member), object(a_object), amount(a_amount) {}

  void run() { (object->*member)(amount); }
};

static int decay(int value) { return value + 1; }

int main() {
  int a01 = decay(0);
  int a02 = decay(a01);
  int a03 = decay(a02);
  int a04 = decay(a03);
  int a05 = decay(a04);
  int a06 = decay(a05);
  int a07 = decay(a06);
  int a08 = decay(a07);
  int a09 = decay(a08);
  int a10 = decay(a09);

  receiver object;
  holder made(decay_copy(&receiver::add), decay_copy(&object), decay_copy(7));
  made.run();

  int sum = a01 + a02 + a03 + a04 + a05 + a06 + a07 + a08 + a09 + a10;
  if (sum != 55) {
    return 1;
  }
  if (object.value != 7) {
    return 2;
  }
  return 0;
}
