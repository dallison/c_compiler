#include <stdint.h>
#include <string.h>
#include <syscall.h>

enum { CHRONO_ABBREV_MAX = 16 };

typedef struct {
  int64_t begin_seconds;
  int64_t end_seconds;
  int32_t offset_seconds;
  uint32_t flags;
  uint32_t reserved;
} ChronoSysInfo;

typedef struct {
  int32_t result;
  uint32_t reserved;
  ChronoSysInfo first;
  ChronoSysInfo second;
} ChronoLocalInfo;

typedef struct {
  int64_t date_seconds;
  int32_t correction_seconds;
  uint32_t reserved;
} ChronoLeapSecond;

enum {
  CHRONO_LOCAL_NONEXISTENT = 1,
};

int main(void) {
  char version[64];
  char current[64];
  char canonical[64];
  char abbrevs[32];
  uint32_t zone_count = 0;
  uint32_t zone_index = 0;
  uint64_t generation = 0;
  uint32_t leap_count = 0;
  ChronoSysInfo sys_info;
  ChronoLocalInfo local_info;
  ChronoLeapSecond leap_info;
  int64_t sys_seconds = 1700000000;
  int64_t local_seconds = 1711844000;

  if (syscall(SYS_TZDB_VERSION, version, sizeof(version)) != 0) return 1;
  if (version[0] == '\0') return 2;
  if (syscall(SYS_TZDB_ZONE_COUNT, &zone_count) != 0) return 3;
  if (zone_count < 4) return 4;
  if (syscall(SYS_TZDB_LOCATE_ZONE, "Alias", canonical, sizeof(canonical),
              &zone_index) != 0)
    return 5;
  if (strcmp(canonical, "FixedOffset") != 0) return 6;
  sys_seconds = 100;
  if (syscall(SYS_TZDB_SYS_INFO, "FixedOffset", &sys_seconds, &sys_info, abbrevs,
              CHRONO_ABBREV_MAX) != 0)
    return 7;
  if (sys_info.offset_seconds != 3600) return 8;
  if (strcmp(abbrevs, "FIX") != 0) return 9;
  sys_seconds = 1712000000;
  if (syscall(SYS_TZDB_SYS_INFO, "DST", &sys_seconds, &sys_info, abbrevs,
              CHRONO_ABBREV_MAX) != 0)
    return 10;
  if (sys_info.offset_seconds != 3600) return 11;
  if (syscall(SYS_TZDB_LOCAL_INFO, "DST", &local_seconds, &local_info, abbrevs,
              CHRONO_ABBREV_MAX) != 0)
    return 12;
  if (local_info.result != CHRONO_LOCAL_NONEXISTENT) return 13;
  if (syscall(SYS_TZDB_GENERATION, &generation) != 0 || generation == 0)
    return 14;
  if (syscall(SYS_TZDB_RELOAD, &generation) != 0 || generation <= 1) return 15;
  if (syscall(SYS_TZDB_CURRENT_ZONE, current, sizeof(current)) != 0) return 16;
  if (current[0] == '\0') return 17;
  if (syscall(SYS_TZDB_LEAP_COUNT, &leap_count) != 0 || leap_count != 1)
    return 18;
  if (syscall(SYS_TZDB_LEAP_INFO, 0, &leap_info) != 0 ||
      leap_info.correction_seconds != 1)
    return 19;
  return 0;
}
