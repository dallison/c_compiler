// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <cmath>
#include <ranges>

int main() {
  constexpr double pi = 3.14159265358979323846;
  int degrees[] = {0,  15, 30, 45, 60,  75,  90,
                   105, 120, 135, 150, 165, 180};
  int degree_sum = 0;
  for (int degree : degrees) {
    degree_sum += degree;
  }
  if (std::ranges::size(degrees) != 13) return 1;
  if (degree_sum != 1170) return 2;

#if defined(__p_code__)
  return 0;
#else
  double sine_values[13] = {};
#if defined(__6502__)
  auto samples = std::views::iota(0, 13);
#endif

  int sample_index = 0;
#if defined(__6502__)
  for (int step : samples) {
    int degree = step * 15;
#else
  for (int degree : degrees) {
#endif
    double radians = degree * pi / 180.0;
    sine_values[sample_index++] = std::sin(radians);
  }

  double sum = 0.0;
  double sum_of_squares = 0.0;
  for (double value : sine_values) {
    sum += value;
    sum_of_squares += value * value;
  }

  double count = static_cast<double>(std::ranges::size(sine_values));
  double average = sum / count;
  double rms = std::sqrt(sum_of_squares / count);

  if (sample_index != 13 || std::ranges::size(sine_values) != 13) return 1;
#if defined(__6502__)
  if (sine_values[0] != 0.0) return 2;
#else
  if (sine_values[0] < -0.01 || sine_values[0] > 0.01) return 2;
#endif
  if (sine_values[1] < 0.25 || sine_values[1] > 0.27) return 3;
  if (sine_values[2] < 0.49 || sine_values[2] > 0.51) return 4;
  if (sine_values[3] < 0.70 || sine_values[3] > 0.72) return 5;
  if (sine_values[4] < 0.85 || sine_values[4] > 0.88) return 6;
  if (sine_values[5] < 0.95 || sine_values[5] > 0.98) return 7;
  if (sine_values[6] < 0.99 || sine_values[6] > 1.01) return 8;
  if (sine_values[7] < 0.95 || sine_values[7] > 0.98) return 9;
  if (sine_values[8] < 0.85 || sine_values[8] > 0.88) return 10;
  if (sine_values[9] < 0.70 || sine_values[9] > 0.72) return 11;
  if (sine_values[10] < 0.49 || sine_values[10] > 0.51) return 12;
  if (sine_values[11] < 0.25 || sine_values[11] > 0.27) return 13;
  if (sine_values[12] < -0.01 || sine_values[12] > 0.01) return 14;
  if (sum < 7.5 || sum > 7.7) return 15;
  if (sum_of_squares < 5.9 || sum_of_squares > 6.1) return 16;
  if (average < 0.58 || average > 0.59) return 17;
  if (rms < 0.67 || rms > 0.69) return 18;
  return 0;
#endif
}
