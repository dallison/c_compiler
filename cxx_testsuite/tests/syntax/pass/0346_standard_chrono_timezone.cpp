// RUN: -std=c++20

#include <chrono>

using namespace std::chrono;

void use_chrono_timezone_types(
    const sys_info& sys, const local_info& local, choose choice,
    const time_zone& zone, const time_zone_link& link, const leap_second& leap,
    const tzdb& database, const tzdb_list& list, const zoned_seconds& zoned,
    const nonexistent_local_time& missing, const ambiguous_local_time& fold) {
  (void)sys.begin;
  (void)sys.end;
  (void)sys.offset;
  (void)sys.save;
  (void)sys.abbrev[0];
  (void)local.info_result;
  (void)local.first;
  (void)local.second;
  (void)choice;
  (void)zone.name();
  (void)link.name();
  (void)link.target();
  (void)leap.date();
  (void)leap.value();
  (void)database.version;
  (void)database.zones;
  (void)database.links;
  (void)database.leap_seconds;
  (void)list.front();
  (void)list.begin();
  (void)list.end();
  (void)zoned.get_time_zone();
  (void)zoned.get_local_time();
  (void)zoned.get_sys_time();
  (void)missing.what();
  (void)fold.what();
}

void use_chrono_timezone_api() {
  (void)get_tzdb();
  (void)get_tzdb_list();
  (void)reload_tzdb();
  (void)remote_version();
  (void)locate_zone("UTC");
  (void)current_zone();
}
