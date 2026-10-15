// Passing a class by value to overloads taking its bases, the conversion to
// the more derived base is better ([over.ics.rank]/4.4): abseil's
// `time_internal` tags derive from each level of cctz's tag hierarchy, so
// `align(T{}, f)` picks the most specific `align`.
namespace cctz {
namespace detail {
struct second_tag {};
struct minute_tag : second_tag {};
struct hour_tag : minute_tag {};
struct day_tag : hour_tag {};
struct month_tag : day_tag {};
struct year_tag : month_tag {};
inline int align(second_tag, int f) { return f; }
inline int align(minute_tag, int f) { return f + 1; }
inline int align(hour_tag, int f) { return f + 2; }
inline int align(day_tag, int f) { return f + 3; }
inline int align(month_tag, int f) { return f + 4; }
inline int align(year_tag, int f) { return f + 5; }
template <typename T>
struct civil {
  int f_;
  explicit civil(int f) : f_(align(T{}, f)) {}
};
}  // namespace detail
}  // namespace cctz

namespace ti {
struct second_tag : cctz::detail::second_tag {};
struct minute_tag : second_tag, cctz::detail::minute_tag {};
struct hour_tag : minute_tag, cctz::detail::hour_tag {};
struct day_tag : hour_tag, cctz::detail::day_tag {};
struct month_tag : day_tag, cctz::detail::month_tag {};
struct year_tag : month_tag, cctz::detail::year_tag {};
}  // namespace ti

int main() {
  using cctz::detail::civil;
  if (civil<ti::second_tag>(1).f_ != 1) return 1;
  if (civil<ti::minute_tag>(1).f_ != 2) return 2;
  if (civil<ti::hour_tag>(1).f_ != 3) return 3;
  if (civil<ti::day_tag>(1).f_ != 4) return 4;
  if (civil<ti::month_tag>(1).f_ != 5) return 5;
  if (civil<ti::year_tag>(1).f_ != 6) return 6;
  if (civil<cctz::detail::day_tag>(1).f_ != 4) return 7;
  return 0;
}
