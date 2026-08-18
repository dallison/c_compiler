// RUN: -std=c++26

template <typename T>
concept Accepts = true;

template <typename, template <typename> concept>
struct Wrapper {};

template <typename... Ts, template <typename> concept... Constraints>
int invalid(Wrapper<Ts, Constraints>...)
  requires (Constraints<Ts> && ...);
