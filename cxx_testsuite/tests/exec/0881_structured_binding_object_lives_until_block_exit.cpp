// The object a structured binding declaration introduces lives until its block
// exits, like any other local; it is not a temporary of the initializer's
// full-expression (Subspace's `auto [resp, fds] = conn.CreatePublisher("ch")`
// read a destroyed `std::string`).
#include <string>
#include <utility>
#include <vector>

static int live = 0;

struct Tracked {
  int id;
  explicit Tracked(int i) : id(i) { live++; }
  Tracked(const Tracked& other) : id(other.id) { live++; }
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

static Response MakeWithArguments(const Tracked& a,
                                  const Tracked& b = Tracked(2)) {
  return Response{Tracked(a.id + b.id), 0};
}

static std::pair<std::string, std::vector<int>> MakePair(
    const std::string& name, const std::string& suffix = "",
    const std::string& mux = "") {
  return {name + suffix + mux, {1, 2, 3, 4}};
}

static int EarlyReturn(bool early) {
  auto [tracked, which] = Make(5);
  if (early) return tracked.id;
  return 0;
}

template <typename F>
static int InTemplate(F make) {
  auto [tracked, which] = make();
  return live == 1 ? tracked.id : -1;
}

int main() {
  {
    auto [tracked, which] = Make(1);
    if (tracked.id != 1 || live != 1) return 1;
  }
  if (live != 0) return 2;
  {
    auto [tracked, which] = MakeWithArguments(Tracked(1));
    if (tracked.id != 3 || live != 1) return 3;
  }
  if (live != 0) return 4;
  {
    auto [name, fds] = MakePair("a_channel_name_longer_than_the_small_buffer");
    if (name != "a_channel_name_longer_than_the_small_buffer" ||
        fds.size() != 4)
      return 5;
  }
  for (int i = 0; i < 3; i++) {
    auto [tracked, which] = Make(i);
    if (tracked.id != i || live != 1) return 6;
    if (i == 1) break;
    if (i == 0) continue;
  }
  if (live != 0) return 7;
  if (EarlyReturn(true) != 5 || live != 0) return 8;
  if (EarlyReturn(false) != 0 || live != 0) return 9;
  {
    std::vector<std::pair<Tracked, int>> items;
    items.emplace_back(Tracked(7), 1);
    items.emplace_back(Tracked(8), 2);
    int sum = 0;
    for (auto [tracked, n] : items) {
      if (live != 3) return 10;
      sum += tracked.id * n;
    }
    if (sum != 23 || live != 2) return 11;
  }
  if (live != 0) return 12;
  {
    Response r = Make(4);
    auto& [tracked, which] = r;
    auto [copy, copy_which] = r;
    if (tracked.id != 4 || copy.id != 4 || live != 2) return 13;
  }
  if (live != 0) return 14;
  {
    auto&& [tracked, which] = Make(6);
    if (tracked.id != 6 || live != 1) return 15;
  }
  if (live != 0) return 16;
  if (auto [tracked, which] = Make(9); tracked.id == 9) {
    if (live != 1) return 17;
  } else {
    return 18;
  }
  if (live != 0) return 19;
  if (InTemplate([] { return Make(10); }) != 10 || live != 0) return 20;
  return 0;
}
