#include <cmath>
#include <cstdio>
#include <ranges>

#if defined(__6502__)
#define PRINT_VALUE(value)                         \
  do {                                             \
    std::printf("[");                              \
    for (int marker = 1; marker <= 20; ++marker) { \
      if ((value) >= marker * 0.05) {              \
        std::printf("#");                          \
      }                                            \
    }                                              \
    std::printf("]");                              \
  } while (false)
#else
#define PRINT_VALUE(value) std::printf("%.6f", value)
#endif

static int degrees[] = {0,  15, 30, 45, 60,  75,  90,
                        105, 120, 135, 150, 165, 180};
static double sine_values[13] = {};

int main() {
  constexpr double pi = 3.14159265358979323846;
  auto steps = std::views::iota(0, 13);

  int sample_index = 0;
  for (int step : steps) {
    int degree = step * 15;
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

  std::printf("sin(x), sampled every 15 degrees:\n");
  for (int index = 0; index < 13; ++index) {
    std::printf("%3d degrees: ", degrees[index]);
    PRINT_VALUE(sine_values[index]);
    std::printf("\n");
  }
  std::printf("samples: %zu\n", std::ranges::size(sine_values));
  std::printf("average: ");
  PRINT_VALUE(average);
  std::printf("\nrms: ");
  PRINT_VALUE(rms);
  std::printf("\n");
  return 0;
}
