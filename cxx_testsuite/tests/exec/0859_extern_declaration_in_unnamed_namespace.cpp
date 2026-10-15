// An `extern` declaration in an unnamed namespace ahead of its definition
// (abseil's charconv.cc `kPower10MantissaHighTable`) names an entity with
// internal linkage, like the definition that follows it, so the two are the
// same variable rather than a linkage mismatch.
namespace {

extern const int kTable[];
extern int initialized = 5;

int Lookup(int i) { return kTable[i] + initialized; }

const int kTable[] = {1, 2, 3};

int Twice(int v);
int Twice(int v) { return 2 * v; }

}  // namespace

int main() {
  if (Lookup(2) != 8) {
    return 1;
  }
  return Twice(4) == 8 ? 0 : 2;
}
