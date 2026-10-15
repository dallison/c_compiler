// A hidden friend of a class template constructs the befriending
// specialization through the injected class name (cctz's civil_time
// `operator+` returns `civil_time(step(T{}, a.f_, n))`), reaching a private
// constructor.
struct fields {
  long y;
  int m;
};
struct second_tag {};
struct minute_tag : second_tag {};

constexpr fields step(second_tag, fields f, long n) {
  return fields{f.y + n, f.m};
}
constexpr fields align(second_tag, fields f) { return f; }

template <typename T>
class civil_time {
 public:
  explicit constexpr civil_time(long y, int m = 1) noexcept
      : civil_time(fields{y, m}) {}
  constexpr civil_time() noexcept : f_{1970, 1} {}
  template <typename U>
  constexpr civil_time(const civil_time<U>& ct) noexcept : civil_time(ct.f_) {}
  long year() const { return f_.y; }

  friend constexpr civil_time operator+(civil_time a, long n) noexcept {
    return civil_time(step(T{}, a.f_, n));
  }
  friend constexpr civil_time operator-(civil_time a, long n) noexcept {
    return n != 0 ? civil_time(step(T{}, a.f_, -n))
                  : civil_time(step(T{}, step(T{}, a.f_, 1), -1));
  }

 private:
  template <typename U>
  friend class civil_time;
  explicit constexpr civil_time(fields f) noexcept : f_(align(T{}, f)) {}
  fields f_;
};

using civil_second = civil_time<second_tag>;
using civil_minute = civil_time<minute_tag>;

int main() {
  civil_second s(2000, 3);
  civil_second t = s + 5;
  civil_second u = t - 2;
  if (t.year() != 2005 || u.year() != 2003 || (u - 0).year() != 2003) {
    return 1;
  }
  civil_minute m = civil_minute(1999) + 1;
  civil_second from_minute = m;
  return from_minute.year() == 2000 ? 0 : 2;
}
