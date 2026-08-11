// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <chrono>
#include <cstring>
#include <format>
#include <string>

using namespace std::chrono;

static int check_nonexistent_to_sys(const time_zone* zone) {
  const local_seconds gap(seconds(1711844000));
  try {
    (void)zone->to_sys(gap);
    return 1;
  } catch (const nonexistent_local_time&) {
    return 0;
  } catch (...) {
    return 2;
  }
}

static int check_ambiguous_choose(const time_zone* zone) {
  const local_seconds fold(seconds(1730593800));
  const local_info info = zone->get_info(fold);
  if (info.info_result != local_info_result::ambiguous) {
    return 0;
  }
  const sys_seconds earliest = zone->to_sys(fold, choose::earliest);
  const sys_seconds latest = zone->to_sys(fold, choose::latest);
  if (earliest.time_since_epoch().count() >= latest.time_since_epoch().count()) {
    return 3;
  }
  return 0;
}

int main() {
  const tzdb& database = get_tzdb();
  if (database.version.empty()) {
    return 10;
  }
  if (get_tzdb_list().begin() == get_tzdb_list().end()) {
    return 11;
  }
  if (remote_version() != database.version) {
    return 12;
  }

  const time_zone* utc = locate_zone("UTC");
  if (utc == nullptr || strcmp(utc->name().data(), "UTC") != 0) {
    return 20;
  }

  const sys_seconds probe(seconds(1700000000));
  const sys_info utc_info = utc->get_info(probe);
  if (utc_info.offset.count() != 0) {
    return 21;
  }

  const zoned_seconds zoned(probe, utc);
  if (zoned.get_sys_time().time_since_epoch() != probe.time_since_epoch()) {
    return 22;
  }
  if (static_cast<sys_seconds>(zoned).time_since_epoch() != probe.time_since_epoch()) {
    return 23;
  }
  if (std::format("{:%F %T %Z %z}", zoned) !=
      "2023-11-14 22:13:20 UTC +0000") {
    return 24;
  }

  const time_zone* current = current_zone();
  if (current == nullptr || current->name().empty()) {
    return 30;
  }

  const time_zone* alias = locate_zone("Alias");
  if (alias == nullptr || strcmp(alias->name().data(), "FixedOffset") != 0) {
    return 40;
  }
  const sys_info fixed = alias->get_info(sys_seconds(seconds(100)));
  if (fixed.offset.count() != 3600 || strcmp(fixed.abbrev, "FIX") != 0) {
    return 41;
  }

  const time_zone* dst = locate_zone("DST");
  if (dst == nullptr) {
    return 50;
  }
  const sys_info dst_summer = dst->get_info(sys_seconds(seconds(1712000000)));
  if (dst_summer.offset.count() != 3600) {
    return 51;
  }
  const local_info gap = dst->get_info(local_seconds(seconds(1711844000)));
  if (gap.info_result != local_info_result::nonexistent) {
    return 52;
  }
  if (check_nonexistent_to_sys(dst) != 0) {
    return 53;
  }
  if (check_ambiguous_choose(dst) != 0) {
    return 54;
  }

  bool found_link = false;
  for (const time_zone_link& link : database.links) {
    if (strcmp(link.name().data(), "Alias") == 0 &&
        strcmp(link.target().data(), "FixedOffset") == 0) {
      found_link = true;
      break;
    }
  }
  if (!found_link) {
    return 60;
  }

  if (database.leap_seconds.empty()) {
    return 70;
  }
  if (database.leap_seconds.front().value().count() != 1) {
    return 71;
  }

  const tzdb& reloaded = reload_tzdb();
  if (reloaded.version.empty()) {
    return 80;
  }
  if (get_tzdb_list().front().version != reloaded.version) {
    return 81;
  }

  zoned_seconds from_local(local_seconds(seconds(100)), alias);
  if (from_local.get_time_zone()->name() != alias->name()) {
    return 90;
  }

  return 0;
}
