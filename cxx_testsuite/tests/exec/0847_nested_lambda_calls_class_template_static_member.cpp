// A lambda nested in a lambda inside a static member of a class template
// calls the class's static member `to_slot` (absl raw_hash_set::move_common).
// The inner closure's parent is the outer closure, so the class is two
// lexical parents up; the call must bind to Set<IntPolicy>::to_slot, and the
// deduced `transfer<char>` must see its concrete argument.
struct Common {
  char buf[8];
  void* data() { return buf; }
  template <typename F>
  void Run(F f) {
    f();
  }
};

template <typename Policy>
struct Traits {
  using slot_type = typename Policy::slot_type;
  template <class Alloc>
  static void transfer(Alloc* alloc, slot_type* dst, slot_type* src) {
    *dst = *src + (int)sizeof(*alloc);
  }
};

struct IntPolicy {
  using slot_type = int;
};

template <typename Policy>
struct Set {
  using slot_type = typename Policy::slot_type;
  using PolicyTraits = Traits<Policy>;
  static slot_type* to_slot(void* buf) { return static_cast<slot_type*>(buf); }
  static void move_common(char& alloc, Common& lhs, Common& rhs) {
    rhs.Run([&] {
      lhs.Run([&] {
        PolicyTraits::transfer(&alloc, to_slot(lhs.data()),
                               to_slot(rhs.data()));
      });
    });
  }
};

int main() {
  Common a, b;
  *(int*)b.buf = 41;
  char alloc = 0;
  Set<IntPolicy>::move_common(alloc, a, b);
  return *(int*)a.buf == 42 ? 0 : 1;
}
