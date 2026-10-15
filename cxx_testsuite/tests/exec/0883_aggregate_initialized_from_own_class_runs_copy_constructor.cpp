// Initializing an aggregate from one object of its own class -- `A y(x)`,
// `A y{x}`, `A(x)` -- runs the copy or move constructor.  Copying the bytes
// left two strings owning one buffer (Subspace's
// `RegisteredClientBuffer{.metadata = std::move(metadata), ...}`).
#include <string>
#include <utility>

struct Named {
  std::string name;
  int k;
};

struct Wrapper {
  Named inner;
  int extra;
};

struct Point {
  int x, y;
};

static const char* const kLong = "a_channel_name_longer_than_the_small_buffer";

static Named Make() { return Named{kLong, 3}; }

int main() {
  {
    Named x{kLong, 1};
    Named y(std::move(x));
    if (y.name != kLong || x.name.data() == y.name.data()) return 1;
  }
  {
    Named x{kLong, 1};
    Named y{std::move(x)};
    if (y.name != kLong || x.name.data() == y.name.data()) return 2;
  }
  {
    const Named x{kLong, 2};
    Named y(x);
    if (y.name != kLong || x.name != kLong || y.k != 2 ||
        x.name.data() == y.name.data())
      return 3;
  }
  {
    Named x{kLong, 1};
    Named y = Named(x);
    Named z = Named(std::move(x));
    if (y.name != kLong || z.name != kLong || y.name.data() == z.name.data())
      return 4;
  }
  {
    Named y = Named(Make());
    if (y.name != kLong || y.k != 3) return 5;
  }
  {
    Wrapper x{{kLong, 1}, 2};
    Wrapper y(std::move(x));
    if (y.inner.name != kLong || y.extra != 2 ||
        x.inner.name.data() == y.inner.name.data())
      return 6;
  }
  {
    Point p{1, 2};
    Point q = Point(p);
    Point r(p);
    Point s{p};
    if (q.y != 2 || r.y != 2 || s.y != 2) return 7;
  }
  {
    Named y = Named{kLong, 5};
    Named z{Named{kLong, 6}};
    if (y.k != 5 || z.k != 6 || z.name != kLong) return 8;
  }
  return 0;
}
