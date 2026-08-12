// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <random>
#include <sstream>
#include <cstdlib>

int main() {
  char* end = nullptr;
  double zero = std::strtod("0", &end);
  if (zero != 0.0) return 10;
  if (end == nullptr || *end != '\0') return 11;

  std::stringstream direct;
  direct << 0.0 << ' ' << 1.0;
  double first = -1.0;
  double second = -1.0;
  direct >> first >> second;
  if (!direct) return 1;
  if (first != 0.0) return 2;
  if (second != 1.0) return 3;

  std::stringstream bool_stream;
  bool_stream << false;
  bool bool_value = true;
  bool_stream >> bool_value;
  if (!bool_stream) return 18;
  if (bool_value) return 19;

  std::stringstream normal_fields("0 1 0 0");
  double normal_mean = -1;
  double normal_stddev = -1;
  bool normal_saved = true;
  double normal_saved_value = -1;
  normal_fields >> normal_mean;
  if (!normal_fields) return 20;
  normal_fields >> normal_stddev;
  if (!normal_fields) return 21;
  normal_fields >> normal_saved;
  if (!normal_fields) return 22;
  normal_fields >> normal_saved_value;
  if (!normal_fields) return 23;
  if (normal_mean != 0.0 || normal_stddev != 1.0 || normal_saved ||
      normal_saved_value != 0.0)
    return 24;

  std::uniform_real_distribution<> source;
  std::stringstream stream;
  stream << source;
  std::uniform_real_distribution<> restored;
  stream >> restored;
  if (!stream) return 4;
  if (source.a() != restored.a()) return 5;
  if (source.b() != restored.b()) return 6;
  if (source != restored) return 7;

  std::normal_distribution<> normal_source;
  std::stringstream normal_stream;
  normal_stream << normal_source;
  std::normal_distribution<> normal_restored;
  normal_stream >> normal_restored;
  if (!normal_stream) return 12;
  if (normal_source != normal_restored) return 13;

  std::lognormal_distribution<> lognormal_source;
  std::stringstream lognormal_stream;
  lognormal_stream << lognormal_source;
  std::lognormal_distribution<> lognormal_restored;
  lognormal_stream >> lognormal_restored;
  if (!lognormal_stream) return 14;
  if (lognormal_source.m() != lognormal_restored.m()) return 15;
  if (lognormal_source.s() != lognormal_restored.s()) return 16;
  return lognormal_source == lognormal_restored ? 0 : 17;
}
