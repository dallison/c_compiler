// A block-scope function declaration names the function of the innermost
// enclosing namespace, so it links to that function's (mangled) definition.
namespace ns {
int twice(int x) { return 2 * x; }
int run() {
  int twice(int);
  return twice(4);
}
int run_out_of_line();
}  // namespace ns

int ns::run_out_of_line() {
  int twice(int);
  return twice(5);
}

int over(int) { return 1; }
int over(long) { return 2; }
int twice(int x) { return x + x; }

int main() {
  int twice(int);
  if (twice(3) != 6) return 1;
  if (ns::run() != 8) return 2;
  if (ns::run_out_of_line() != 10) return 3;
  int over(long);
  if (over(1) != 2) return 4;
  return 0;
}
