// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>
#include <sstream>

template <class Engine>
int check_engine_stream(int error) {
  Engine source(17);
  source.discard(9);
  std::stringstream stream;
  stream << source;
  Engine restored;
  stream >> restored;
  if (!stream || source != restored) return error;
  return source() == restored() ? 0 : error;
}

template <class Distribution>
int check_distribution_stream(int error) {
  Distribution source;
  std::stringstream stream;
  stream << source;
  Distribution restored;
  stream >> restored;
  return stream && source == restored ? 0 : error;
}

int main() {
  if (int rc = check_engine_stream<std::minstd_rand>(1)) return rc;
  if (int rc = check_engine_stream<std::mt19937>(2)) return rc;
  if (int rc = check_engine_stream<std::ranlux24_base>(3)) return rc;
  if (int rc = check_engine_stream<std::ranlux24>(4)) return rc;
  if (int rc = check_engine_stream<
          std::independent_bits_engine<std::minstd_rand, 12, unsigned>>(5))
    return rc;
  if (int rc = check_engine_stream<std::knuth_b>(6)) return rc;

  if (int rc = check_distribution_stream<std::uniform_int_distribution<>>(10))
    return rc;
  if (int rc =
          check_distribution_stream<std::uniform_real_distribution<>>(11))
    return rc;
  if (int rc = check_distribution_stream<std::bernoulli_distribution>(12))
    return rc;
  if (int rc = check_distribution_stream<std::geometric_distribution<>>(13))
    return rc;
  if (int rc = check_distribution_stream<std::binomial_distribution<>>(14))
    return rc;
  if (int rc =
          check_distribution_stream<std::negative_binomial_distribution<>>(15))
    return rc;
  if (int rc = check_distribution_stream<std::poisson_distribution<>>(16))
    return rc;
  if (int rc = check_distribution_stream<std::exponential_distribution<>>(17))
    return rc;
  if (int rc = check_distribution_stream<std::gamma_distribution<>>(18))
    return rc;
  if (int rc = check_distribution_stream<std::lognormal_distribution<>>(19))
    return rc;
  if (int rc = check_distribution_stream<std::weibull_distribution<>>(20))
    return rc;
  if (int rc =
          check_distribution_stream<std::extreme_value_distribution<>>(21))
    return rc;
  if (int rc =
          check_distribution_stream<std::chi_squared_distribution<>>(22))
    return rc;
  if (int rc = check_distribution_stream<std::cauchy_distribution<>>(23))
    return rc;
  if (int rc = check_distribution_stream<std::fisher_f_distribution<>>(24))
    return rc;
  if (int rc = check_distribution_stream<std::student_t_distribution<>>(25))
    return rc;
  if (int rc = check_distribution_stream<std::discrete_distribution<>>(26))
    return rc;
  if (int rc =
          check_distribution_stream<std::piecewise_constant_distribution<>>(27))
    return rc;
  if (int rc =
          check_distribution_stream<std::piecewise_linear_distribution<>>(28))
    return rc;

  std::minstd_rand engine(31);
  std::normal_distribution<> normal(2.0, 3.0);
  (void)normal(engine);
  std::stringstream stream;
  stream << normal;
  std::normal_distribution<> restored;
  stream >> restored;
  if (!stream || !(normal == restored)) return 29;
  return normal(engine) == restored(engine) ? 0 : 30;
}
