// RUN: -std=c++20
// EXPECT: template argument pack expansion requires a parameter pack

template <class T>
struct Box {
};

template <class... Ts>
struct Tuple {
};

template <class T>
struct BadTemplateArgumentPattern {
  Tuple<Box<int>...> value;
};

BadTemplateArgumentPattern<int> bad_template_argument_pattern;
