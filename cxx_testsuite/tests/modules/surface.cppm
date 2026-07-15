export module surface;

export namespace mod {
export enum class Color : int { Red = 1, Green = 2, Blue = 3 };

export using IntAlias = int;

template <typename T>
concept Integral = sizeof(T) >= sizeof(char);
export template <typename T>
concept Numeric = Integral<T>;

export struct Box {
  int value;
};

export constexpr int k_answer = 42;

export template <typename T>
struct Holder {
  T value;
};

export inline namespace version {
export int inline_ns_value() { return 99; }
}
}

export int pick(int x, int y) { return x + y; }
export int pick_def(int x, int y = 1) { return x + y; }
export int one(int x) { return x; }
export double one(double x) { return x; }
export template <typename T> T id(T x) { return x; }
export template <> int id(int x) { return x; }
export template <mod::Numeric T> T add_one(T x) { return x + T(1); }
