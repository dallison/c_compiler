// RUN: -std=c++26

template <typename T>
concept Valid = sizeof(T) > 0;

template <typename... Ts>
  requires (Valid<Ts> && ...)
int choose(Ts...);

template <typename... Ts>
  requires (Valid<Ts> || ...)
int choose(Ts...);

int value = choose(1);
