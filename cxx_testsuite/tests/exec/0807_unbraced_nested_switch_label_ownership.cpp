// RUN: -std=c++17
// EXPECT_EXIT: 0

// An unbraced nested switch body (Abseil's logging idiom
// `switch (0) case 0: default: stmt;`) owns only its chained labels.  Labels
// after its statement belong to the enclosing switch, whose controlling type
// may be a scoped enumeration.

enum class Kind { kCord, kView, kString, kOther };

#define LOG_LIKE(counter) \
  switch (0)              \
  case 0:                 \
  default:                \
    counter += 100

int Classify(Kind kind) {
  int counter = 0;
  switch (kind) {
    case Kind::kCord:
      LOG_LIKE(counter);
    case Kind::kView:
    case Kind::kString:
      return counter + 1;
    case Kind::kOther:
      break;
  }
  return counter + 2;
}

int Chained(int outer, int inner) {
  int r = 0;
  switch (outer) {
    case 1:
      switch (inner)
      case 10:
      case 11:
      default:
        r += 10;
    case 2:
      r += 1;
      break;
    case 3:
      switch (inner)
      default:;
      r += 5;
      break;
    case 4:
      switch (inner) {
        case 0:
          r += 20;
      }
    default:
      r += 3;
  }
  return r;
}

int main() {
  if (Classify(Kind::kCord) != 101) return 1;
  if (Classify(Kind::kView) != 1) return 2;
  if (Classify(Kind::kString) != 1) return 3;
  if (Classify(Kind::kOther) != 2) return 4;
  if (Chained(1, 10) != 11) return 5;
  if (Chained(1, 99) != 11) return 6;
  if (Chained(2, 0) != 1) return 7;
  if (Chained(3, 0) != 5) return 8;
  if (Chained(4, 0) != 23) return 9;
  if (Chained(4, 1) != 3) return 10;
  if (Chained(9, 0) != 3) return 11;
  return 0;
}
