// RUN: -std=c++20
template <typename T>
concept Always = true;

template <typename T, int N>
concept SizeAtLeast = sizeof(T) >= N;

template <typename T>
concept Large = sizeof(T) > 4;

template <SizeAtLeast<1> T>
int explicit_concept_arg(T value) {
  return sizeof(value);
}

template <Large T = long long>
int constrained_default(void) {
  return sizeof(T);
}

template <Always... Ts>
int constrained_pack_size(void) {
  return sizeof...(Ts);
}

int main(void) {
  return explicit_concept_arg((char)0) + constrained_default<>() +
         constrained_pack_size<int, char, long long>() - 12;
}
