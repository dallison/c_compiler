// A CRTP base's member template may access a member of the derived class
// that is its template argument; the access resolves once the derived class
// is complete (abseil's `HashStateBase<H>::combine` updating `state.n`).
#include <utility>

template <typename H>
struct StateBase {
  template <typename T>
  static H Add(H state, const T& value) {
    state.n += static_cast<int>(value);
    return state;
  }
  template <typename T, typename... Ts>
  static H AddAll(H state, const T& value, const Ts&... values) {
    state = Add(std::move(state), value);
    if constexpr (sizeof...(values) > 0) {
      return AddAll(std::move(state), values...);
    } else {
      return state;
    }
  }
};

template <typename T>
struct Spy : StateBase<Spy<T>> {
  int n = 0;
  static Spy Make(int start) {
    Spy s;
    s.n = start;
    return Spy::AddAll(std::move(s), 1, 2L, 3.0);
  }
};

struct Plain : StateBase<Plain> {
  int n = 10;
};

int main() {
  if (Spy<void>::Make(0).n != 6) return 1;
  if (Spy<int>::Make(4).n != 10) return 2;
  Plain p;
  if (Plain::AddAll(p, 5).n != 15) return 3;
  return 0;
}
