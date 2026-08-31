// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <numeric>

struct add {
    int operator()(int a, int b) const { return a + b; }
};

struct multiply {
    int operator()(int a, int b) const { return a * b; }
};

struct square {
    int operator()(int value) const { return value * value; }
};

static bool same(const int *values, const int *expected, int size) {
    for (int i = 0; i < size; ++i) {
        if (values[i] != expected[i]) return false;
    }
    return true;
}

int main() {
    int values[4] = {};
    std::iota(values, values + 4, 1);
    if (std::accumulate(values, values + 4, 0) != 10) return 1;
    if (std::accumulate(values, values + 4, 1, multiply()) != 24) return 2;

    int weights[] = {4, 3, 2, 1};
    if (std::inner_product(values, values + 4, weights, 0) != 20) return 3;
    if (std::inner_product(values, values + 4, weights, 1, add(),
                           multiply()) != 21) return 4;

    int output[4];
    const int sums[] = {1, 3, 6, 10};
    std::partial_sum(values, values + 4, output);
    if (!same(output, sums, 4)) return 5;
    const int products[] = {1, 2, 6, 24};
    std::partial_sum(values, values + 4, output, multiply());
    if (!same(output, products, 4)) return 6;

    const int differences[] = {1, 1, 1, 1};
    std::adjacent_difference(values, values + 4, output);
    if (!same(output, differences, 4)) return 7;

    if (std::gcd(-42, 30) != 6) return 8;
    if (std::lcm(-6, 15) != 30) return 24;
    if (std::reduce(values, values + 4) != 10) return 9;
    if (std::reduce(values, values + 4, 5, add()) != 15) return 10;
    if (std::transform_reduce(values, values + 4, weights, 0) != 20)
        return 11;
    if (std::transform_reduce(values, values + 4, 0, add(), square()) != 30)
        return 12;

    const int exclusive[] = {10, 11, 13, 16};
    std::exclusive_scan(values, values + 4, output, 10);
    if (!same(output, exclusive, 4)) return 13;
    std::inclusive_scan(values, values + 4, output);
    if (!same(output, sums, 4)) return 14;

    const int transformed_exclusive[] = {0, 1, 5, 14};
    std::transform_exclusive_scan(values, values + 4, output, 0, add(),
                                  square());
    if (!same(output, transformed_exclusive, 4)) return 15;
    const int transformed_inclusive[] = {1, 5, 14, 30};
    std::transform_inclusive_scan(values, values + 4, output, add(), square());
    if (!same(output, transformed_inclusive, 4)) return 16;

    if (std::midpoint(3, 10) != 6 || std::midpoint(10, 3) != 7) return 17;
    if (std::midpoint(values, values + 4) != values + 2) return 18;
    if (std::midpoint(2.0, 4.0) != 3.0) return 19;
    if (std::midpoint(std::numeric_limits<int>::min(),
                      std::numeric_limits<int>::max()) != -1)
        return 25;
    double largest = std::numeric_limits<double>::max();
    if (std::midpoint(largest, largest) != largest) return 26;

    if (std::saturate_cast<unsigned char>(-1) != 0) return 20;
    if (std::saturate_cast<signed char>(1000) != 127) return 21;
    if (std::saturate_cast<signed char>(-1000) != -128) return 22;
    if (std::saturate_cast<unsigned char>(1000) != 255) return 23;

    return 0;
}
