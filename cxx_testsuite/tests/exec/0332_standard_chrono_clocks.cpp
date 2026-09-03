// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <chrono>

using namespace std::chrono;

int main() {
  const auto sys_before = system_clock::now();
  const auto steady_before = steady_clock::now();
  const auto file_before = file_clock::now();
  const auto utc_before = utc_clock::now();
  const auto tai_before = tai_clock::now();
  const auto gps_before = gps_clock::now();
  const local_time<seconds> local_value(seconds(3));
  if (local_value.time_since_epoch().count() != 3) {
    return 10;
  }

  if (sys_before.time_since_epoch().count() <= 0) {
    return 1;
  }
  if (steady_before.time_since_epoch().count() < 0) {
    return 2;
  }

  const sys_time<system_clock::duration> round_trip =
      utc_clock::to_sys(utc_clock::from_sys(sys_before));
  if (round_trip.time_since_epoch() != sys_before.time_since_epoch()) {
    return 4;
  }

  const sys_time<system_clock::duration> from_file =
      file_clock::to_sys(file_before);
  const auto file_round_trip = file_clock::from_sys(from_file);
  if (duration_cast<nanoseconds>(file_before.time_since_epoch()).count() !=
      duration_cast<nanoseconds>(file_round_trip.time_since_epoch()).count()) {
    return 5;
  }

  const utc_time<utc_clock::duration> utc_from_tai =
      tai_clock::to_utc(tai_before);
  const auto tai_round_trip = tai_clock::from_utc(utc_from_tai);
  if (duration_cast<microseconds>(tai_before.time_since_epoch()).count() !=
      duration_cast<microseconds>(tai_round_trip.time_since_epoch()).count()) {
    return 6;
  }

  const utc_time<utc_clock::duration> utc_from_gps =
      gps_clock::to_utc(gps_before);
  const auto gps_round_trip = gps_clock::from_utc(utc_from_gps);
  if (duration_cast<microseconds>(gps_before.time_since_epoch()).count() !=
      duration_cast<microseconds>(gps_round_trip.time_since_epoch()).count()) {
    return 7;
  }

  const auto tai_via_cast = clock_cast<tai_clock>(sys_before);
  const auto gps_via_cast = clock_cast<gps_clock>(sys_before);
  if (tai_via_cast.time_since_epoch().count() <= sys_before.time_since_epoch().count()) {
    return 8;
  }
  if (gps_via_cast.time_since_epoch().count() <= sys_before.time_since_epoch().count()) {
    return 9;
  }

  (void)utc_before;
  (void)file_before;
  return 0;
}
