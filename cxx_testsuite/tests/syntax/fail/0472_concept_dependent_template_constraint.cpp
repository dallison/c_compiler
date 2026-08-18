// RUN: -std=c++26

template <typename T>
concept Accepts = true;

template <template <typename> concept Constraint,
          template <Constraint T> typename Template>
struct Invalid {};
