#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <time.h>

enum {
  kTimeZoneNameCapacity = 256,
  kTimeZoneAbbreviationCapacity = 16,
};

typedef struct {
  int64_t begin_seconds;
  int64_t end_seconds;
  int32_t offset_seconds;
  uint32_t flags;
  uint32_t reserved;
} TimeZoneSysInfo;

typedef struct {
  uint64_t seconds_address;
  uint64_t result_address;
  uint64_t abbreviation_address;
  uint32_t abbreviation_capacity;
  uint32_t reserved;
} TimeZoneSysInfoRequest;

#if defined(__DAVECC_HAS_TLS_THREAD_ERRNO__)
static __thread char local_time_zone[kTimeZoneAbbreviationCapacity];
#else
static char local_time_zone[kTimeZoneAbbreviationCapacity];
#endif

static int64_t DaysFromCivil(int64_t year, unsigned month, unsigned day) {
  year -= month <= 2;
  int64_t era = (year >= 0 ? year : year - 399) / 400;
  unsigned year_of_era = (unsigned)(year - era * 400);
  int adjusted_month = (int)month + (month > 2 ? -3 : 9);
  unsigned day_of_year =
      (unsigned)(((int64_t)153 * adjusted_month + 2) / 5 + day - 1);
  int64_t day_of_era = (int64_t)year_of_era * 365 + year_of_era / 4 -
                       year_of_era / 100 + day_of_year;
  return era * 146097 + day_of_era - 719468;
}

static int BrokenDownTime(int64_t seconds, struct tm* result) {
  int64_t days = seconds / 86400;
  int64_t remainder = seconds % 86400;
  if (remainder < 0) {
    remainder += 86400;
    --days;
  }

  int64_t adjusted_days = days + 719468;
  int64_t era =
      (adjusted_days >= 0 ? adjusted_days : adjusted_days - 146096) / 146097;
  int64_t day_of_era = adjusted_days - era * 146097;
  int64_t year_of_era =
      (day_of_era - day_of_era / 1460 + day_of_era / 36524 -
       day_of_era / 146096) /
      365;
  int64_t year = year_of_era + era * 400;
  int64_t day_of_year =
      day_of_era -
      (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
  int64_t month_prime = (5 * day_of_year + 2) / 153;
  unsigned day =
      (unsigned)(day_of_year - (153 * month_prime + 2) / 5 + 1);
  unsigned month = (unsigned)(month_prime < 10 ? month_prime + 3
                                               : month_prime - 9);
  year += month <= 2;

  int64_t tm_year = year - 1900;
  if ((int64_t)(int)tm_year != tm_year) {
    errno = EOVERFLOW;
    return -1;
  }

  result->tm_sec = (int)(remainder % 60);
  result->tm_min = (int)((remainder / 60) % 60);
  result->tm_hour = (int)(remainder / 3600);
  result->tm_mday = (int)day;
  result->tm_mon = (int)month - 1;
  result->tm_year = (int)tm_year;
  int64_t week_day = (days + 4) % 7;
  if (week_day < 0) {
    week_day += 7;
  }
  result->tm_wday = (int)week_day;
  result->tm_yday = (int)(days - DaysFromCivil(year, 1, 1));
  return 0;
}

#if defined(__DAVECC_HAS_HOST_TZDB__)
static int ReadTimeZoneInfo(const char* zone_name, int64_t seconds,
                            TimeZoneSysInfo* info, char* abbreviation,
                            size_t abbreviation_capacity) {
#if defined(__6502__)
  return (int)syscall(SYS_TZDB_SYS_INFO, zone_name, &seconds, info,
                      abbreviation, abbreviation_capacity);
#else
  TimeZoneSysInfoRequest request = {
      (uint64_t)(uintptr_t)&seconds, (uint64_t)(uintptr_t)info,
      (uint64_t)(uintptr_t)abbreviation, (uint32_t)abbreviation_capacity, 0};
  return (int)syscall(SYS_TZDB_SYS_INFO, zone_name, &request);
#endif
}
#endif

int timespec_get(struct timespec* result, int base) {
  if (base != TIME_UTC || result == NULL) {
    return 0;
  }

  int64_t microseconds = 0;
#if defined(__DAVECC_NATIVE_LINUX__)
  struct timespec native_time;
  if (syscall(SYS_clock_gettime, 0, &native_time) < 0) {
    return 0;
  }
  result->tv_sec = native_time.tv_sec;
  result->tv_nsec = native_time.tv_nsec;
  return TIME_UTC;
#elif defined(__DAVECC_HAS_HOST_CLOCK__)
  if (syscall(SYS_REALTIME_TIME, &microseconds) != 0) {
    return 0;
  }
#else
  microseconds = (int64_t)time(NULL) * 1000000;
#endif
  int64_t seconds = microseconds / 1000000;
  int64_t remainder = microseconds % 1000000;
  if (remainder < 0) {
    remainder += 1000000;
    --seconds;
  }
  result->tv_sec = (time_t)seconds;
  if ((int64_t)result->tv_sec != seconds) {
    return 0;
  }
  result->tv_nsec = (long)(remainder * 1000);
  return TIME_UTC;
}

int timespec_getres(struct timespec* result, int base) {
  if (base != TIME_UTC) {
    return 0;
  }
  if (result != NULL) {
#if defined(__DAVECC_HAS_HOST_CLOCK__)
    result->tv_sec = 0;
    result->tv_nsec = 1000;
#else
    result->tv_sec = 1;
    result->tv_nsec = 0;
#endif
  }
  return TIME_UTC;
}

struct tm* gmtime_r(const time_t* time_point, struct tm* result) {
  if (BrokenDownTime((int64_t)*time_point, result) != 0) {
    return NULL;
  }
  result->tm_isdst = 0;
  result->tm_gmtoff = 0;
  result->tm_zone = "UTC";
  return result;
}

struct tm* localtime_r(const time_t* time_point, struct tm* result) {
#if defined(__DAVECC_HAS_HOST_TZDB__)
  char zone_name[kTimeZoneNameCapacity];
  if (syscall(SYS_TZDB_CURRENT_ZONE, zone_name, sizeof(zone_name)) < 0) {
    return gmtime_r(time_point, result);
  }

  TimeZoneSysInfo info;
  if (ReadTimeZoneInfo(zone_name, (int64_t)*time_point, &info, local_time_zone,
                       sizeof(local_time_zone)) < 0) {
    return gmtime_r(time_point, result);
  }

  int64_t seconds = (int64_t)*time_point;
  int64_t maximum = (int64_t)(~(uint64_t)0 >> 1);
  int64_t minimum = -maximum - 1;
  if ((info.offset_seconds > 0 &&
       seconds > maximum - info.offset_seconds) ||
      (info.offset_seconds < 0 &&
       seconds < minimum - info.offset_seconds)) {
    errno = EOVERFLOW;
    return NULL;
  }
  seconds += info.offset_seconds;
  if (BrokenDownTime(seconds, result) != 0) {
    return NULL;
  }
  result->tm_isdst = (info.flags & 1u) != 0;
  result->tm_gmtoff = info.offset_seconds;
  result->tm_zone = local_time_zone;
  return result;
#else
  return gmtime_r(time_point, result);
#endif
}

static struct tm broken_down_time;
static char formatted_time[26];

struct tm* gmtime(const time_t* time_point) {
  return gmtime_r(time_point, &broken_down_time);
}

struct tm* localtime(const time_t* time_point) {
  return localtime_r(time_point, &broken_down_time);
}

char* asctime_r(const struct tm* value, char* buffer) {
  static const char* const week_days[] = {
      "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char* const months[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  if (value == NULL || buffer == NULL || value->tm_wday < 0 ||
      value->tm_wday >= 7 || value->tm_mon < 0 || value->tm_mon >= 12) {
    errno = EINVAL;
    return NULL;
  }
  int length =
      snprintf(buffer, 26, "%s %s %2d %02d:%02d:%02d %d\n",
               week_days[value->tm_wday], months[value->tm_mon],
               value->tm_mday, value->tm_hour, value->tm_min, value->tm_sec,
               value->tm_year + 1900);
  if (length < 0 || length >= 26) {
    errno = EOVERFLOW;
    return NULL;
  }
  return buffer;
}

char* asctime(const struct tm* value) {
  return asctime_r(value, formatted_time);
}

char* ctime_r(const time_t* time_point, char* buffer) {
  struct tm value;
  if (time_point == NULL || localtime_r(time_point, &value) == NULL) {
    return NULL;
  }
  return asctime_r(&value, buffer);
}

char* ctime(const time_t* time_point) {
  return ctime_r(time_point, formatted_time);
}
