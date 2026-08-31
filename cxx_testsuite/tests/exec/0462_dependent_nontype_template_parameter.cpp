// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// A non-type template parameter may spell its type as a dependent qualified
// name.  There `typename` is a disambiguator, not the type-parameter keyword,
// so the parameter list must not be read as declaring a type parameter.

template <class T>
struct Traits {
    typedef int value_type;
};

template <class T>
struct Widths {
    typedef long value_type;
};

// Dependent qualified parameter type, no default.
template <class T, typename Traits<T>::value_type N>
struct Fixed {
    static const int value = N;
};

// Dependent qualified parameter type with a default.
template <class T, typename Traits<T>::value_type N = 7>
struct Defaulted {
    static const int value = N;
};

// A different dependent qualified type, so the parameter type is really read
// from the trait rather than assumed to be int.
template <class T, typename Widths<T>::value_type N>
struct Wide {
    static const long value = N;
};

// Non-dependent qualified parameter type, which took the same parse path.
template <class T, typename Traits<int>::value_type N = 3>
struct Concrete {
    static const int value = N;
};

// Function template with a dependent qualified parameter type.
template <class T, typename Traits<T>::value_type N>
int scaled(int factor) {
    return static_cast<int>(N) * factor;
}

// The type-parameter spellings must all still work alongside it.
template <typename A, typename B = char, typename... Rest>
struct StillTypeParameters {
    static const int count = 2 + static_cast<int>(sizeof...(Rest));
};

static_assert(Fixed<char, 4>::value == 4, "dependent parameter");
static_assert(Defaulted<char>::value == 7, "dependent default");
static_assert(Defaulted<char, 9>::value == 9, "dependent explicit");
static_assert(Wide<char, 100000L>::value == 100000L, "wide dependent");
static_assert(Concrete<char>::value == 3, "non-dependent qualified");
static_assert(StillTypeParameters<int, long, float, double>::count == 4,
              "type parameters");

int main() {
    if (Fixed<short, 5>::value != 5) return 1;
    if (Defaulted<short>::value != 7) return 2;
    if (scaled<short, 6>(3) != 18) return 3;
    if (Wide<int, 5L>::value != 5L) return 4;
    if (StillTypeParameters<int>::count != 2) return 5;
    return 0;
}
