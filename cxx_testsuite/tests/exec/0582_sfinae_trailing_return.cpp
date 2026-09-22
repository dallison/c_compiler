// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

struct Spec {};
struct Sink {};
struct SpecImpl {};
struct SinkImpl {};

void AbslFormatConvert();
void AbslStringify();

template <typename T>
auto FormatConvertImpl(const T& v, SpecImpl, SinkImpl*)
    -> decltype(AbslFormatConvert(v, std::declval<const Spec&>(),
                                  std::declval<Sink*>())) {
  return {};
}

using IntegralConvertResult = int;

template <typename T>
auto FormatConvertImpl(const T& v, SpecImpl, SinkImpl*)
    -> std::enable_if_t<std::is_enum<T>::value &&
                            std::is_void<decltype(AbslStringify(
                                std::declval<Sink&>(), v))>::value,
                        IntegralConvertResult> {
  return 1;
}

template <typename T>
auto FormatConvertImpl(const T&, SpecImpl, SinkImpl*)
    -> std::enable_if_t<!std::is_enum<T>::value, IntegralConvertResult> {
  return 2;
}

enum E { kE };

int main() {
  return FormatConvertImpl(0, SpecImpl{}, nullptr) == 2 &&
                 FormatConvertImpl(kE, SpecImpl{}, nullptr) == 1
             ? 0
             : 1;
}
