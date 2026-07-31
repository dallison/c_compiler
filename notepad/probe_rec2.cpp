using size_t = decltype(sizeof(0));

template <size_t N, size_t... Values>
struct sequence : sequence<N - 1, N - 1, Values...> {};

template <size_t... Values>
struct sequence<0, Values...> {};

sequence<3> value;
