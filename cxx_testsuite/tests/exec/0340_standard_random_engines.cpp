// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>

template <class Engine, class Value>
int check_count(Value expected, int error, int count = 10000) {
  Engine engine;
  Value value = 0;
  for (int i = 0; i < count; ++i) {
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
  if (int rc = check_count<std::minstd_rand0>(16807U, 1, 1)) return rc;
  if (int rc = check_count<std::minstd_rand>(48271U, 2, 1)) return rc;
  if (int rc = check_count<std::mt19937>(3499211612U, 3, 1)) return rc;
  if (int rc =
          check_count<std::mt19937_64>(14514284786278117030ULL, 4, 1)) {
    return rc;
  }
  if (int rc = check_count<std::ranlux24_base>(15039276U, 5, 1)) return rc;
  if (int rc =
          check_count<std::ranlux48_base>(23459059301164ULL, 6, 1)) {
    return rc;
  }
  // Crossing one block boundary verifies each adaptor's discard behavior
  // without making interpreted cross-target tests execute hundreds of
  // thousands of intentionally discarded base-engine values.
  if (int rc = check_count<std::ranlux24>(15059233U, 7, 24)) return rc;
  if (int rc =
          check_count<std::ranlux48>(269312768919532ULL, 8, 12)) {
    return rc;
  }
  if (int rc = check_count<std::knuth_b>(152607844U, 9, 1)) return rc;
#endif
  unsigned seeds[] = {1, 2, 3, 4};
  std::seed_seq first_seed(seeds, seeds + 4);
  std::seed_seq second_seed(seeds, seeds + 4);
  unsigned first_values[8] = {};
  unsigned second_values[8] = {};
  first_seed.generate(first_values, first_values + 8);
  second_seed.generate(second_values, second_values + 8);
  for (int i = 0; i < 8; ++i) {
    if (first_values[i] != second_values[i]) return 10;
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
