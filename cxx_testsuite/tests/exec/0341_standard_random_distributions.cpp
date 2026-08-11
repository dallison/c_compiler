// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>

int main() {
  std::minstd_rand engine(12345);

  std::uniform_int_distribution<int> ui(-7, 11);
  std::uniform_real_distribution<double> ur(-2.0, 3.0);
  std::bernoulli_distribution bernoulli(0.3);
  std::binomial_distribution<int> binomial(12, 0.4);
  std::geometric_distribution<int> geometric(0.4);
  std::negative_binomial_distribution<int> negative_binomial(4, 0.4);
  std::poisson_distribution<int> poisson(3.0);
  std::exponential_distribution<double> exponential(2.0);
  std::gamma_distribution<double> gamma(2.0, 3.0);
  std::weibull_distribution<double> weibull(2.0, 3.0);
  std::extreme_value_distribution<double> extreme(1.0, 2.0);
  std::normal_distribution<double> normal(4.0, 2.0);
  std::lognormal_distribution<double> lognormal(0.0, 0.5);
  std::chi_squared_distribution<double> chi(3.0);
  std::cauchy_distribution<double> cauchy(1.0, 2.0);
  std::fisher_f_distribution<double> fisher(3.0, 5.0);
  std::student_t_distribution<double> student(6.0);

  double weights[] = {1.0, 2.0, 3.0};
  std::discrete_distribution<int> discrete(weights, weights + 3);
  double boundaries[] = {0.0, 1.0, 4.0};
  std::piecewise_constant_distribution<double> piecewise_constant(
      boundaries, boundaries + 3, weights);
  std::piecewise_linear_distribution<double> piecewise_linear(
      boundaries, boundaries + 3, weights);

  for (int i = 0; i < 40; ++i) {
    int ivalue = ui(engine);
    if (ivalue < -7 || ivalue > 11) return 1;
    double rvalue = ur(engine);
    if (!(rvalue >= -2.0 && rvalue < 3.0)) return 2;
    if (binomial(engine) < 0 || binomial(engine) > 12) return 3;
    if (geometric(engine) < 0 || negative_binomial(engine) < 0) return 4;
    if (poisson(engine) < 0) return 5;
    if (!(exponential(engine) >= 0.0)) return 6;
    if (!(gamma(engine) >= 0.0)) return 7;
    if (!(weibull(engine) >= 0.0)) return 8;
    if (!(lognormal(engine) >= 0.0)) return 9;
    if (!(chi(engine) >= 0.0)) return 10;
    if (!(fisher(engine) >= 0.0)) return 11;
    int category = discrete(engine);
    if (category < 0 || category > 2) return 12;
    double pc = piecewise_constant(engine);
    if (!(pc >= 0.0 && pc < 4.0)) return 13;
    double pl = piecewise_linear(engine);
    if (!(pl >= 0.0 && pl < 4.0)) return 14;
    (void)bernoulli(engine);
    (void)extreme(engine);
    (void)normal(engine);
    (void)cauchy(engine);
    (void)student(engine);
  }

  if (discrete.probabilities().size() != 3) return 15;
  if (piecewise_constant.intervals().size() != 3) return 16;
  if (piecewise_linear.densities().size() != 3) return 17;
  return 0;
}
