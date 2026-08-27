// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// Three defects in the -O2 inliner, all of which `std::invoke` walks straight
// into.  A wrapper forwarding to a void callee is written `return callee(...);`,
// and the inliner turned a return statement into a jump to the epilogue label
// while discarding its operand -- correct for the result of a value-returning
// call, but the void call itself was the only thing the statement did.  A
// reference parameter bound to a prvalue argument gets a materialized
// temporary, whose address is what the reference holds, so that temporary must
// not be register allocated.  And a pointer-to-member-function call lowers to a
// dereference of the loaded pointer whose type record is the member function's
// own -- and therefore carries its body -- so inlining it called that one
// member rather than the pointed-to one.

struct receiver {
  int value = 0;

  void add(int increment) { value += increment; }
  void subtract(int decrement) { value -= decrement; }
};

static void PlainAdd(receiver* object, int increment) {
  object->value += increment;
}

template <class Function, class... Args>
static constexpr void CallFunction(Function&& function, Args&&... args) {
  return static_cast<Function&&>(function)(static_cast<Args&&>(args)...);
}

template <class Member, class Object, class... Args>
static void CallMember(Member& member, Object object, Args&&... args) {
  return ((*object).*member)(static_cast<Args&&>(args)...);
}

template <class Member, class Object, class... Args>
static void ForwardToMember(Member&& member, Object&& object, Args&&... args) {
  return CallMember<Member, Object>(member, static_cast<Object&&>(object),
                                    static_cast<Args&&>(args)...);
}

int main() {
  // A void call as the operand of a return statement in an inlined wrapper.
  // `&direct` is a prvalue, so the forwarded reference parameter binds to a
  // materialized temporary holding it.
  receiver direct;
  void (*function)(receiver*, int) = PlainAdd;
  CallFunction(function, &direct, 7);
  if (direct.value != 7) {
    return 1;
  }

  // Which member the pointer names is only known to the caller.
  receiver adjusted;
  ForwardToMember(&receiver::add, &adjusted, 10);
  if (adjusted.value != 10) {
    return 2;
  }
  ForwardToMember(&receiver::subtract, &adjusted, 4);
  if (adjusted.value != 6) {
    return 3;
  }

  // The same wrapper reached with the other member, so a body inlined from the
  // first instantiation would show up here.
  void (receiver::*selected)(int) = &receiver::subtract;
  ForwardToMember(selected, &adjusted, 6);
  if (adjusted.value != 0) {
    return 4;
  }

  return 0;
}
