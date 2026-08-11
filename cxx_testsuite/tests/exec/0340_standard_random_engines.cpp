// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>

template <class Engine, class Value>
int check_10000(Value expected, int error) {
  Engine engine;
  Value value = 0;
  for (int i = 0; i < 10000; ++i) {
    value = static_cast<Value>(engine());
  }
  return value == expected ? 0 : error;
}

int main() {
#if defined(__6502__)
  std::minstd_rand0 minstd0;
  if (minstd0() != 16807U) return 1;
  std::minstd_rand minstd;
  if (minstd() != 48271U) return 2;
  std::mt19937 mt;
  if (mt() != 3499211612U) return 3;
  std::mt19937_64 mt64;
  if (mt64() != 14514284786278117030ULL) return 4;
  std::ranlux24_base ranlux_base;
  if (ranlux_base() > std::ranlux24_base::max()) return 5;
  std::ranlux24 ranlux;
  if (ranlux() > std::ranlux24::max()) return 7;
  std::knuth_b knuth;
  if (knuth() > std::knuth_b::max()) return 9;
#else
  if (int rc = check_10000<std::minstd_rand0>(1043618065U, 1)) return rc;
  if (int rc = check_10000<std::minstd_rand>(399268537U, 2)) return rc;
  if (int rc = check_10000<std::mt19937>(4123659995U, 3)) return rc;
  if (int rc =
          check_10000<std::mt19937_64>(9981545732273789042ULL, 4)) {
    return rc;
  }
  if (int rc = check_10000<std::ranlux24_base>(7937952U, 5)) return rc;
  if (int rc =
          check_10000<std::ranlux48_base>(61839128582725ULL, 6)) {
    return rc;
  }
  if (int rc = check_10000<std::ranlux24>(9901578U, 7)) return rc;
  if (int rc = check_10000<std::ranlux48>(249142670248501ULL, 8)) {
    return rc;
  }
  if (int rc = check_10000<std::knuth_b>(1112339016U, 9)) return rc;
#endif

  unsigned seeds[] = {1, 2, 3, 4};
  std::seed_seq first_seed(seeds, seeds + 4);
  std::seed_seq second_seed(seeds, seeds + 4);
  std::mt19937 first(first_seed);
  std::mt19937 second(second_seed);
  for (int i = 0; i < 10; ++i) {
    if (first() != second()) return 10;
  }

  std::independent_bits_engine<std::minstd_rand, 12, unsigned> bits(7);
  for (int i = 0; i < 20; ++i) {
    if (bits() > 4095U) return 11;
  }

  std::minstd_rand canonical_engine(19);
  double canonical =
      std::generate_canonical<double, 53>(canonical_engine);
  if (!(canonical >= 0.0 && canonical < 1.0)) return 12;

  std::minstd_rand discard_a(23);
  std::minstd_rand discard_b(23);
  discard_a.discard(37);
  for (int i = 0; i < 37; ++i) discard_b();
  if (discard_a != discard_b) return 13;

  return 0;
}
