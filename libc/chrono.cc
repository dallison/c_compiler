#include <chrono>
#include <cstring>
#include <syscall.h>

#include <__exception_support>

extern "C" long long __davecc_monotonic_time_us(void);
extern "C" long long __davecc_realtime_time_us(void);

namespace std {
namespace chrono {

namespace __chrono_tz_detail {

[[noreturn]] static void __throw_runtime(const char* message) {
  __DAVECC_THROW(runtime_error(message));
}

#if defined(__DAVECC_HAS_HOST_TZDB__)

enum { __name_max = 256, __abbrev_max = 16 };

struct __wire_sys_info {
  int64_t begin_seconds;
  int64_t end_seconds;
  int32_t offset_seconds;
  uint32_t flags;
  uint32_t reserved;
};

struct __wire_local_info {
  int32_t result;
  uint32_t reserved;
  __wire_sys_info first;
  __wire_sys_info second;
};

struct __wire_leap_second {
  int64_t date_seconds;
  int32_t correction_seconds;
  uint32_t reserved;
};

struct __wire_sys_info_request {
  uint64_t seconds_address;
  uint64_t result_address;
  uint64_t abbrev_address;
  uint32_t abbrev_capacity;
  uint32_t reserved;
};

struct __wire_local_info_request {
  uint64_t seconds_address;
  uint64_t result_address;
  uint64_t abbrev_address;
  uint32_t abbrev_capacity;
  uint32_t reserved;
};

enum {
  __local_unique = 0,
  __local_nonexistent = 1,
  __local_ambiguous = 2,
};

struct __tzdb_state {
  tzdb_list list;
  uint64_t generation;
  bool metadata_loaded;
};

static __tzdb_state& __state() {
  static __tzdb_state state{};
  return state;
}

static void __check_syscall(int64_t result, const char* message) {
  if (result < 0) {
    __throw_runtime(message);
  }
}

static void __copy_name(string_view name, char* buffer, size_t capacity) {
  if (name.size() + 1 > capacity) {
    __throw_runtime("time zone name too long");
  }
  memcpy(buffer, name.data(), name.size());
  buffer[name.size()] = '\0';
}

static sys_seconds __seconds_from_wire(int64_t value) {
  return sys_seconds(seconds(value));
}

static sys_info __sys_info_from_wire(const __wire_sys_info& wire,
                                     const char* abbrev) {
  sys_info info;
  info.begin = __seconds_from_wire(wire.begin_seconds);
  info.end = __seconds_from_wire(wire.end_seconds);
  info.offset = seconds(wire.offset_seconds);
  info.save = (wire.flags & 1u) != 0 ? 3600 : 0;
  strncpy(info.abbrev, abbrev, sizeof(info.abbrev));
  info.abbrev[sizeof(info.abbrev) - 1] = '\0';
  return info;
}

static int64_t __sys_info_syscall(const char* zone_name, int64_t sys_seconds_val,
                                  __wire_sys_info* wire, char* abbrev,
                                  size_t abbrev_capacity) {
#if defined(__6502__)
  return syscall(SYS_TZDB_SYS_INFO, zone_name, &sys_seconds_val, wire, abbrev,
                 abbrev_capacity);
#else
  __wire_sys_info_request request = {
      (uint64_t)(uintptr_t)&sys_seconds_val, (uint64_t)(uintptr_t)wire,
      (uint64_t)(uintptr_t)abbrev, (uint32_t)abbrev_capacity, 0};
  return syscall(SYS_TZDB_SYS_INFO, zone_name, &request);
#endif
}

static int64_t __local_info_syscall(const char* zone_name,
                                    int64_t local_seconds_val,
                                    __wire_local_info* wire, char* first_abbrev,
                                    char* second_abbrev,
                                    size_t abbrev_capacity) {
#if defined(__6502__)
  return syscall(SYS_TZDB_LOCAL_INFO, zone_name, &local_seconds_val, wire,
                 first_abbrev, abbrev_capacity);
#else
  __wire_local_info_request request = {
      (uint64_t)(uintptr_t)&local_seconds_val, (uint64_t)(uintptr_t)wire,
      (uint64_t)(uintptr_t)first_abbrev, (uint32_t)abbrev_capacity, 0};
  (void)second_abbrev;
  return syscall(SYS_TZDB_LOCAL_INFO, zone_name, &request);
#endif
}

static local_info_result __local_result_from_wire(int32_t value) {
  switch (value) {
    case __local_nonexistent:
      return local_info_result::nonexistent;
    case __local_ambiguous:
      return local_info_result::ambiguous;
    default:
      return local_info_result::unique;
  }
}

static void __load_version(tzdb& database) {
  char buffer[64];
  __check_syscall(syscall(SYS_TZDB_VERSION, buffer, sizeof(buffer)),
                    "get_tzdb version");
  database.version = buffer;
}

static void __load_leap_seconds(tzdb& database) {
  database.leap_seconds.clear();
  uint32_t count = 0;
  int32_t previous_correction = 0;
  __check_syscall(syscall(SYS_TZDB_LEAP_COUNT, &count),
                    "get_tzdb leap count");
  for (uint32_t index = 0; index < count; ++index) {
    __wire_leap_second wire{};
    __check_syscall(syscall(SYS_TZDB_LEAP_INFO, index, &wire),
                      "get_tzdb leap info");
    const int32_t adjustment =
        wire.correction_seconds - previous_correction;
    previous_correction = wire.correction_seconds;
    database.leap_seconds.emplace_back(
        sys_seconds(seconds(wire.date_seconds)),
        seconds(adjustment));
  }
}

static void __load_links(tzdb& database) {
  database.links.clear();
  uint32_t count = 0;
  __check_syscall(syscall(SYS_TZDB_ZONE_COUNT, &count), "get_tzdb zone count");
  char name_buffer[__name_max];
  char canonical_buffer[__name_max];
  for (uint32_t index = 0; index < count; ++index) {
    __check_syscall(
        syscall(SYS_TZDB_ZONE_NAME, index, name_buffer, sizeof(name_buffer)),
        "get_tzdb zone name");
    uint32_t resolved_index = 0;
    int64_t locate_result =
        syscall(SYS_TZDB_LOCATE_ZONE, name_buffer, canonical_buffer,
                sizeof(canonical_buffer), &resolved_index);
    if (locate_result < 0) {
      continue;
    }
    if (strcmp(name_buffer, canonical_buffer) != 0) {
      database.links.emplace_back(name_buffer, canonical_buffer);
    }
  }
}

static void __load_zone_names(tzdb& database) {
  database.zones.clear();
  uint32_t count = 0;
  __check_syscall(syscall(SYS_TZDB_ZONE_COUNT, &count), "get_tzdb zone count");
  char name_buffer[__name_max];
  char canonical_buffer[__name_max];
  for (uint32_t index = 0; index < count; ++index) {
    __check_syscall(
        syscall(SYS_TZDB_ZONE_NAME, index, name_buffer, sizeof(name_buffer)),
        "get_tzdb zone name");
    uint32_t resolved_index = 0;
    __check_syscall(
        syscall(SYS_TZDB_LOCATE_ZONE, name_buffer, canonical_buffer,
                sizeof(canonical_buffer), &resolved_index),
        "get_tzdb locate zone");
    bool known = false;
    for (const time_zone& zone : database.zones) {
      if (strcmp(zone.name().data(), canonical_buffer) == 0) {
        known = true;
        break;
      }
    }
    if (!known) {
      database.zones.emplace_back(canonical_buffer);
    }
  }
}

static void __ensure_metadata(tzdb& database) {
  __tzdb_state& state = __state();
  uint64_t generation = 0;
  __check_syscall(syscall(SYS_TZDB_GENERATION, &generation),
                    "get_tzdb generation");
  if (state.metadata_loaded && generation == state.generation &&
      !database.version.empty()) {
    return;
  }
  __load_version(database);
  __load_leap_seconds(database);
  __load_links(database);
  __load_zone_names(database);
  state.generation = generation;
  state.metadata_loaded = true;
}

static tzdb& __current_database() {
  __tzdb_state& state = __state();
  if (state.list.__entries_mut().empty()) {
    state.list.__entries_mut().emplace_back();
  }
  tzdb& database = state.list.__entries_mut().front();
  __ensure_metadata(database);
  return database;
}

static const time_zone& __intern_zone(tzdb& database, string_view name) {
  for (const time_zone& zone : database.zones) {
    if (zone.name() == name) {
      return zone;
    }
  }
  char buffer[__name_max];
  __copy_name(name, buffer, sizeof(buffer));
  database.zones.emplace_back(buffer);
  return database.zones.back();
}

sys_info __time_zone_sys_info(const time_zone& zone, sys_seconds tp) {
  __wire_sys_info wire{};
  char abbrev[__abbrev_max];
  char zone_name[__name_max];
  __copy_name(zone.name(), zone_name, sizeof(zone_name));
  int64_t seconds = tp.time_since_epoch().count();
  __check_syscall(__sys_info_syscall(zone_name, seconds, &wire, abbrev,
                                     sizeof(abbrev)),
                    "time_zone::get_info(sys_time)");
  return __sys_info_from_wire(wire, abbrev);
}

local_info __time_zone_local_info(const time_zone& zone, local_seconds tp) {
  __wire_local_info wire{};
  char abbrevs[__abbrev_max * 2];
  char* first_abbrev = abbrevs;
  char* second_abbrev = abbrevs + __abbrev_max;
  char zone_name[__name_max];
  __copy_name(zone.name(), zone_name, sizeof(zone_name));
  int64_t seconds = tp.time_since_epoch().count();
  __check_syscall(__local_info_syscall(zone_name, seconds, &wire,
                                       first_abbrev, second_abbrev,
                                       __abbrev_max),
                    "time_zone::get_info(local_time)");
  local_info info;
  info.info_result = __local_result_from_wire(wire.result);
  info.first = __sys_info_from_wire(wire.first, first_abbrev);
  info.second = __sys_info_from_wire(wire.second, second_abbrev);
  return info;
}

local_seconds __time_zone_to_local(const time_zone& zone, sys_seconds tp) {
  const sys_info info = __time_zone_sys_info(zone, tp);
  return local_seconds(info.offset + tp.time_since_epoch());
}

sys_seconds __time_zone_to_sys(const time_zone& zone, local_seconds tp,
                               choose selection) {
  const local_info info = __time_zone_local_info(zone, tp);
  if (info.info_result == local_info_result::unique) {
    return sys_seconds(tp.time_since_epoch() - info.first.offset);
  }
  if (info.info_result == local_info_result::nonexistent) {
    __DAVECC_THROW(nonexistent_local_time("nonexistent local time",
                                          string(zone.name().data())));
  }
  if (selection == choose::latest) {
    return sys_seconds(tp.time_since_epoch() - info.second.offset);
  }
  return sys_seconds(tp.time_since_epoch() - info.first.offset);
}

#endif  // __DAVECC_HAS_HOST_TZDB__

}  // namespace __chrono_tz_detail

nonexistent_local_time::nonexistent_local_time(const string& what_arg,
                                               const string& tz_name)
    : runtime_error(what_arg + ": " + tz_name) {}

nonexistent_local_time::nonexistent_local_time(const char* what_arg,
                                               const string& tz_name)
    : runtime_error(string(what_arg) + ": " + tz_name) {}

ambiguous_local_time::ambiguous_local_time(const string& what_arg,
                                           const string& tz_name)
    : runtime_error(what_arg + ": " + tz_name) {}

ambiguous_local_time::ambiguous_local_time(const char* what_arg,
                                           const string& tz_name)
    : runtime_error(string(what_arg) + ": " + tz_name) {}

#if defined(__DAVECC_HAS_HOST_TZDB__)

const tzdb& get_tzdb() {
  return __chrono_tz_detail::__current_database();
}

const tzdb_list& get_tzdb_list() {
  (void)get_tzdb();
  return __chrono_tz_detail::__state().list;
}

const tzdb& reload_tzdb() {
  uint64_t generation = 0;
  __chrono_tz_detail::__check_syscall(
      syscall(SYS_TZDB_RELOAD, &generation), "reload_tzdb");
  __chrono_tz_detail::__tzdb_state& state = __chrono_tz_detail::__state();
  state.metadata_loaded = false;
  state.generation = generation;
  tzdb fresh;
  __chrono_tz_detail::__load_version(fresh);
  __chrono_tz_detail::__load_leap_seconds(fresh);
  __chrono_tz_detail::__load_links(fresh);
  __chrono_tz_detail::__load_zone_names(fresh);
  state.list.__entries_mut().insert(state.list.__entries_mut().begin(),
                                    std::move(fresh));
  state.metadata_loaded = true;
  return state.list.front();
}

string remote_version() { return get_tzdb().version; }

const time_zone* locate_zone(string_view name) {
  return locate_zone(name, __chrono_tz_detail::__current_database());
}

const time_zone* locate_zone(string_view name, tzdb& database) {
  char canonical[__chrono_tz_detail::__name_max];
  char query[__chrono_tz_detail::__name_max];
  if (name.size() >= sizeof(query)) {
    __chrono_tz_detail::__throw_runtime("locate_zone name too long");
  }
  memcpy(query, name.data(), name.size());
  query[name.size()] = '\0';
  uint32_t index = 0;
  __chrono_tz_detail::__check_syscall(
      syscall(SYS_TZDB_LOCATE_ZONE, query, canonical, sizeof(canonical),
              &index),
      "locate_zone");
  return &__chrono_tz_detail::__intern_zone(database, canonical);
}

const time_zone* current_zone() {
  return current_zone(__chrono_tz_detail::__current_database());
}

const time_zone* current_zone(tzdb& database) {
  char buffer[__chrono_tz_detail::__name_max];
  __chrono_tz_detail::__check_syscall(
      syscall(SYS_TZDB_CURRENT_ZONE, buffer, sizeof(buffer)), "current_zone");
  return &__chrono_tz_detail::__intern_zone(database, buffer);
}

#else  // !__DAVECC_HAS_HOST_TZDB__

namespace __chrono_tz_detail {

sys_info __time_zone_sys_info(const time_zone& zone, sys_seconds tp) {
  (void)zone;
  sys_info info;
  info.begin = sys_seconds::min();
  info.end = sys_seconds::max();
  info.offset = seconds(0);
  info.save = 0;
  strncpy(info.abbrev, "UTC", sizeof(info.abbrev));
  info.abbrev[sizeof(info.abbrev) - 1] = '\0';
  (void)tp;
  return info;
}

local_info __time_zone_local_info(const time_zone& zone, local_seconds tp) {
  (void)zone;
  local_info info;
  info.info_result = local_info_result::unique;
  info.first.begin = sys_seconds::min();
  info.first.end = sys_seconds::max();
  info.first.offset = seconds(0);
  info.first.save = 0;
  strncpy(info.first.abbrev, "UTC", sizeof(info.first.abbrev));
  info.first.abbrev[sizeof(info.first.abbrev) - 1] = '\0';
  info.second = info.first;
  (void)tp;
  return info;
}

local_seconds __time_zone_to_local(const time_zone& zone, sys_seconds tp) {
  (void)zone;
  return local_seconds(tp.time_since_epoch());
}

sys_seconds __time_zone_to_sys(const time_zone& zone, local_seconds tp,
                               choose selection) {
  (void)zone;
  (void)selection;
  return sys_seconds(tp.time_since_epoch());
}

}  // namespace __chrono_tz_detail

const tzdb& get_tzdb() {
  static tzdb database{};
  if (database.version.empty()) {
    database.version = "0";
    database.zones.emplace_back("UTC");
  }
  return database;
}

const tzdb_list& get_tzdb_list() {
  static tzdb_list list{};
  if (list.__entries_mut().empty()) {
    list.__entries_mut().push_back(get_tzdb());
  }
  return list;
}

const tzdb& reload_tzdb() { return get_tzdb(); }

string remote_version() { return get_tzdb().version; }

const time_zone* locate_zone(string_view name) {
  return locate_zone(name, const_cast<tzdb&>(get_tzdb()));
}

const time_zone* locate_zone(string_view name, tzdb& database) {
  for (const time_zone& zone : database.zones) {
    if (zone.name() == name) {
      return &zone;
    }
  }
  __chrono_tz_detail::__throw_runtime("locate_zone");
  return nullptr;
}

const time_zone* current_zone() {
  return current_zone(const_cast<tzdb&>(get_tzdb()));
}

const time_zone* current_zone(tzdb& database) {
  for (const time_zone& zone : database.zones) {
    if (zone.name() == "UTC") {
      return &zone;
    }
  }
  database.zones.emplace_back("UTC");
  return &database.zones.back();
}

#endif  // __DAVECC_HAS_HOST_TZDB__

namespace {

microseconds __realtime_since_epoch() noexcept {
  return microseconds(__davecc_realtime_time_us());
}

}  // namespace

system_clock::time_point system_clock::now() noexcept {
  return time_point(__realtime_since_epoch());
}

time_t system_clock::to_time_t(const system_clock::time_point& value) noexcept {
  return static_cast<time_t>(
      duration_cast<seconds>(value.time_since_epoch()).count());
}

system_clock::time_point system_clock::from_time_t(time_t value) noexcept {
  return time_point(seconds(value));
}

steady_clock::time_point steady_clock::now() noexcept {
  return time_point(duration(__davecc_monotonic_time_us()));
}

file_clock::time_point file_clock::now() noexcept {
  return time_point(__realtime_since_epoch());
}

system_clock::time_point file_clock::to_sys(
    const file_clock::time_point& value) noexcept {
  return system_clock::time_point{value.time_since_epoch()};
}

file_clock::time_point file_clock::from_sys(
    const system_clock::time_point& value) noexcept {
  return time_point(value.time_since_epoch());
}

utc_clock::time_point utc_clock::now() noexcept {
  return from_sys(system_clock::now());
}

system_clock::time_point utc_clock::to_sys(
    const utc_clock::time_point& value) noexcept {
  return system_clock::time_point{
      system_clock::duration(value.time_since_epoch().count())};
}

utc_clock::time_point utc_clock::from_sys(
    const system_clock::time_point& value) noexcept {
  return utc_clock::time_point{
      utc_clock::duration(value.time_since_epoch().count())};
}

tai_clock::time_point tai_clock::now() noexcept {
  return from_utc(utc_clock::now());
}

utc_clock::time_point tai_clock::to_utc(
    const tai_clock::time_point& value) noexcept {
  return utc_clock::time_point{
      utc_clock::duration(value.time_since_epoch().count() -
                          __chrono_detail::__tai_minus_utc.count())};
}

tai_clock::time_point tai_clock::from_utc(
    const utc_clock::time_point& value) noexcept {
  return time_point{tai_clock::duration(value.time_since_epoch().count() +
                                        __chrono_detail::__tai_minus_utc.count())};
}

gps_clock::time_point gps_clock::now() noexcept {
  return from_utc(utc_clock::now());
}

utc_clock::time_point gps_clock::to_utc(
    const gps_clock::time_point& value) noexcept {
  return utc_clock::time_point{
      utc_clock::duration(value.time_since_epoch().count() -
                          __chrono_detail::__gps_minus_utc.count())};
}

gps_clock::time_point gps_clock::from_utc(
    const utc_clock::time_point& value) noexcept {
  return time_point{gps_clock::duration(value.time_since_epoch().count() +
                                        __chrono_detail::__gps_minus_utc.count())};
}

}  // namespace chrono
}  // namespace std
