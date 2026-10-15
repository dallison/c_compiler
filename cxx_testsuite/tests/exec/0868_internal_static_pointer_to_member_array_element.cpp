// `static const A* p = &obj.arr[1];` is classified as dynamic initialization
// before analysis (the member array's type is not known yet), and analysis then
// folds it to a link-time constant.  The run-time store must still happen: the
// variable was given zeroed storage, and treating the folded store as a static
// initializer dropped it, leaving the pointer null.
struct A {
  int a;
};
struct Holder {
  int x = 1;
  A arr[2] = {{4}, {5}};
};
struct Plain {
  int x;
  A arr[2];
};
extern Holder gh;
Plain gp;

static const A* parr = &gh.arr[1];
static const A* volatile parr_v = &gh.arr[1];
static const int* pa = &gh.arr[1].a;
static const A* parr_p = &gp.arr[1];
static int early = parr->a;
Holder gh;
static const A* parr_after = &gh.arr[1];

int main() {
  if (parr != &gh.arr[1]) return 1;
  if (parr_v != &gh.arr[1]) return 2;
  if (pa != &gh.arr[1].a) return 3;
  if (parr_p != &gp.arr[1]) return 4;
  if (parr_after != &gh.arr[1]) return 5;
  if (early != 5) return 6;
  return 0;
}
