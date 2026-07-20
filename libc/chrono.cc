#include <chrono>

namespace std {
namespace chrono {

system_clock::time_point system_clock::now() noexcept {
  return time_point(seconds(time(nullptr)));
}

time_t system_clock::to_time_t(const system_clock::time_point& value) noexcept {
  return static_cast<time_t>(
      duration_cast<seconds>(value.time_since_epoch()).count());
}

system_clock::time_point system_clock::from_time_t(time_t value) noexcept {
  return time_point(seconds(value));
}

steady_clock::time_point steady_clock::now() noexcept {
  return time_point(duration(clock() / CLOCKS_PER_SEC));
}

}  // namespace chrono
}  // namespace std
