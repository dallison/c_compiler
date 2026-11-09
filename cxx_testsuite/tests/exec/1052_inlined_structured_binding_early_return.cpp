// RUN: -std=c++20
// EXPECT_EXIT: 0
// A structured binding's hidden object in an inlined function is destroyed
// once on every exit path, including an early return out of the inlined body.
static int live = 0;

struct Tracked {
  int id;
  explicit Tracked(int i) : id(i) { live++; }
  ~Tracked() {
    live--;
    id = -1;
  }
};

struct Response {
  Tracked tracked;
  int which;
};

static Response Make(int i) { return Response{Tracked(i), 0}; }

__attribute__((always_inline)) static inline int EarlyReturn(bool early) {
  auto [tracked, which] = Make(5);
  if (early) return tracked.id;
  return which;
}

int main(int argc, char**) {
  if (EarlyReturn(argc == 1) != 5) return 1;
  if (live != 0) return 2;
  if (EarlyReturn(argc != 1) != 0) return 3;
  if (live != 0) return 4;
  return 0;
}
