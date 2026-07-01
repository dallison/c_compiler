// An unqualified reference to a `static` data member inside a member function
// (including a static member function, which has no `this`) must resolve to the
// class's static member.  Previously only non-static members were reachable
// unqualified (through the implicit `this`).

struct Counter {
  static int live;
  static int total_created;
  int id;

  Counter() {
    // Unqualified static-member access in a (non-static) constructor.
    ++live;
    ++total_created;
    id = total_created;
  }

  ~Counter() {
    // Unqualified static-member access in a destructor.
    --live;
  }

  // Unqualified static-member access in a static member function.
  static int alive() { return live; }
  static void reset() { live = 0; }
};

int Counter::live = 0;
int Counter::total_created = 0;

int main() {
  Counter a;
  Counter b;
  if (Counter::alive() != 2) {
    return 1;
  }
  if (a.id != 1 || b.id != 2) {
    return 2;
  }
  {
    Counter c;
    if (Counter::alive() != 3) {
      return 3;
    }
  }
  // c destroyed: live back to 2.
  if (Counter::alive() != 2) {
    return 4;
  }
  if (Counter::total_created != 3) {
    return 5;
  }
  Counter::reset();
  if (Counter::live != 0) {
    return 6;
  }
  return 0;
}
