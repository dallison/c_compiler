// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>
#include <sstream>

int main() {
  std::minstd_rand engine(31);
  std::normal_distribution<> source;
  double value = source(engine);
  if (value != value) return 1;

  std::stringstream stream;
  stream << source;
  std::normal_distribution<> restored;
  stream >> restored;
  if (!stream) return 2;
  if (source.mean() != restored.mean()) return 3;
  if (source.stddev() != restored.stddev()) return 4;
  if (source != restored) return 5;
  return source(engine) == restored(engine) ? 0 : 6;
}
