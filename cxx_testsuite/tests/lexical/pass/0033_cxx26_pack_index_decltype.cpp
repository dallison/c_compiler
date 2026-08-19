// RUN: -std=c++26

template<class T> struct unparenthesized_tag;
template<> struct unparenthesized_tag<int> {
  char marker;
};

template<class T> struct parenthesized_tag;
template<> struct parenthesized_tag<int&> {
  char marker[2];
};

template<class... Ts>
int check_decltype_category(Ts... values) {
  unparenthesized_tag<decltype(values...[0])> unparenthesized;
  parenthesized_tag<decltype((values...[0]))> parenthesized;
  return sizeof(unparenthesized) + sizeof(parenthesized);
}

int main(void) {
  return check_decltype_category(5) == 3 ? 0 : 1;
}
