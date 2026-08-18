// RUN: -std=c++26

template <typename T>
concept Accepts = true;

template <template <typename> concept Constraint, typename T>
  requires Constraint<T>
int choose(T);

template <template <typename> concept Constraint, typename T>
  requires (Constraint<T> && true)
int choose(T);

int value = choose<Accepts>(1);
