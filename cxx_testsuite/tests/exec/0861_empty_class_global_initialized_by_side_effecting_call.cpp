// A namespace-scope object of an empty class still runs its initializer when
// that is not the no-op default constructor.  abseil's ABSL_FLAG registers
// each flag with `FlagRegistrarEmpty FLAGS_nox = FlagRegistrar<T, true>(...)`,
// whose constructor registers the flag before converting to the empty class.
int registered = 0;

void Register(int value) { registered += value; }

struct Empty {};

template <bool do_register>
class Registrar {
 public:
  constexpr explicit Registrar(int value) {
    if (do_register) Register(value);
  }
  constexpr operator Empty() const { return {}; }
};

struct NoOp {
  NoOp() {}
};

Empty MakeEmpty(int value) {
  Register(value);
  return {};
}

NoOp MakeNoOp(int value) {
  Register(value);
  return {};
}

Empty converted = Registrar<true>(1);
Empty converted_again = Registrar<true>(2);
Empty from_call = MakeEmpty(4);
NoOp no_op_from_call = MakeNoOp(8);
NoOp default_constructed;
Empty skipped = Registrar<false>(16);

int main() { return registered == 15 ? 0 : 1; }
