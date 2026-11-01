// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

int main() {
  std::istringstream huge("1e40");
  float f = 1.5f;
  huge >> f;
  if (!huge.fail()) return 1;
  if (f != std::numeric_limits<float>::infinity()) return 2;

  std::istringstream neg("-1e40");
  float nf = 1.5f;
  neg >> nf;
  if (!neg.fail()) return 3;
  if (nf != -std::numeric_limits<float>::infinity()) return 4;

  std::istringstream okf("2.5");
  float two = 0;
  okf >> two;
  if (!okf || two != 2.5f) return 5;

  std::istringstream tinyf("1e-50");
  float zf = 1.5f;
  tinyf >> zf;
  if (!tinyf.fail()) return 6;
  if (zf != 0.0f) return 7;

  std::istringstream bigd("1e40");
  double d = 0;
  bigd >> d;
  if (!bigd || d < 1e39 || d > 1e41) return 8;

  std::istringstream over("1e309");
  double inf = 1;
  over >> inf;
  if (!over.fail()) return 9;
  if (inf != std::numeric_limits<double>::infinity()) return 10;

  std::istringstream nover("-1e309");
  double ninf = 1;
  nover >> ninf;
  if (!nover.fail()) return 11;
  if (ninf != -std::numeric_limits<double>::infinity()) return 12;

  std::istringstream under("1e-1000");
  double zero = 1;
  under >> zero;
  if (!under.fail()) return 13;
  if (zero != 0.0) return 14;

  std::istringstream okd("3.5");
  double three = 0;
  okd >> three;
  if (!okd || three != 3.5) return 15;

  bool threw = false;
  try {
    (void)std::stod("1e309");
  } catch (const std::out_of_range&) {
    threw = true;
  }
  if (!threw) return 16;

  threw = false;
  try {
    (void)std::stof("1e40");
  } catch (const std::out_of_range&) {
    threw = true;
  }
  if (!threw) return 17;

  if (std::stof("2.5") != 2.5f) return 18;

  std::istringstream sub("1e-320");
  double tiny = 0;
  sub >> tiny;
  if (!sub) return 19;
  if (!(tiny > 0.0) || !(tiny < 1e-300)) return 20;

  std::istringstream finite("1e308");
  double still = 0;
  finite >> still;
  if (!finite) return 21;
  if (!(still > 9.9e307) || !(still < 1.1e308)) return 22;

  // 10^100 * 10^-401 is about 10^-301, which is finite. The exponent is
  // large, but the coefficient keeps the value inside the double range.
  std::string compensated(101, '0');
  compensated[0] = '1';
  compensated += "e-401";
  double scaled = std::stod(compensated);
  if (scaled < 9.9e-302 || scaled > 1.1e-301) return 23;

  return 0;
}
