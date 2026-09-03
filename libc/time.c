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

double difftime(time_t later, time_t earlier) {
  return (double)later - (double)earlier;
}

time_t mktime(struct tm* value) {
  int64_t year;
  int64_t month;
  int64_t seconds;
  time_t result;
  struct tm normalized;
  if (value == NULL) {
    errno = EINVAL;
    return (time_t)-1;
  }
  year = (int64_t)value->tm_year + 1900;
  month = value->tm_mon;
  year += month / 12;
  month %= 12;
  if (month < 0) {
    month += 12;
    year--;
  }
  seconds = DaysFromCivil(year, (unsigned)month + 1, 1) * 86400 +
            (int64_t)(value->tm_mday - 1) * 86400 +
            (int64_t)value->tm_hour * 3600 +
            (int64_t)value->tm_min * 60 + value->tm_sec;
  result = (time_t)seconds;
  if ((int64_t)result != seconds) {
    errno = EOVERFLOW;
    return (time_t)-1;
  }
#if defined(__DAVECC_HAS_HOST_TZDB__)
  if (localtime_r(&result, &normalized) != NULL) {
    seconds -= normalized.tm_gmtoff;
    result = (time_t)seconds;
    if ((int64_t)result != seconds) {
      errno = EOVERFLOW;
      return (time_t)-1;
    }
  }
#endif
  if (localtime_r(&result, &normalized) == NULL) return (time_t)-1;
  *value = normalized;
  return result;
}

static int AppendText(char* output, size_t capacity, size_t* length,
                      const char* text) {
  while (*text != '\0') {
    if (*length + 1 >= capacity) return 0;
    output[(*length)++] = *text++;
  }
  return 1;
}

static int AppendNumber(char* output, size_t capacity, size_t* length,
                        int value, int width, char padding) {
  char buffer[32];
  int used = 0;
  unsigned int magnitude;
  int negative = value < 0;
  magnitude = negative ? (unsigned int)(-(value + 1)) + 1
                       : (unsigned int)value;
  do {
    buffer[used++] = (char)('0' + magnitude % 10);
    magnitude /= 10;
  } while (magnitude != 0);
  if (negative) buffer[used++] = '-';
  while (used < width) buffer[used++] = padding;
  while (used != 0) {
    if (*length + 1 >= capacity) return 0;
    output[(*length)++] = buffer[--used];
  }
  return 1;
}

size_t strftime(char* restrict output, size_t capacity,
                const char* restrict format, const struct tm* restrict value) {
  static const char* const week_days[] = {
      "Sunday", "Monday", "Tuesday", "Wednesday",
      "Thursday", "Friday", "Saturday"};
  static const char* const months[] = {
      "January", "February", "March", "April", "May", "June",
      "July", "August", "September", "October", "November", "December"};
  size_t length = 0;
  if (output == NULL || format == NULL || value == NULL || capacity == 0) {
    return 0;
  }
  while (*format != '\0') {
    char conversion;
    const char* text;
    if (*format != '%') {
      if (length + 1 >= capacity) return 0;
      output[length++] = *format++;
      continue;
    }
    format++;
    conversion = *format++;
    if (conversion == '%') {
      if (length + 1 >= capacity) return 0;
      output[length++] = '%';
    } else if (conversion == 'Y') {
      if (!AppendNumber(output, capacity, &length,
                        value->tm_year + 1900, 4, '0')) return 0;
    } else if (conversion == 'y') {
      if (!AppendNumber(output, capacity, &length,
                        (value->tm_year + 1900) % 100, 2, '0')) return 0;
    } else if (conversion == 'C') {
      if (!AppendNumber(output, capacity, &length,
                        (value->tm_year + 1900) / 100, 2, '0')) return 0;
    } else if (conversion == 'm') {
      if (!AppendNumber(output, capacity, &length,
                        value->tm_mon + 1, 2, '0')) return 0;
    } else if (conversion == 'd') {
      if (!AppendNumber(output, capacity, &length,
                        value->tm_mday, 2, '0')) return 0;
    } else if (conversion == 'e') {
      if (!AppendNumber(output, capacity, &length,
                        value->tm_mday, 2, ' ')) return 0;
    } else if (conversion == 'H') {
      if (!AppendNumber(output, capacity, &length,
                        value->tm_hour, 2, '0')) return 0;
    } else if (conversion == 'I') {
      int hour = value->tm_hour % 12;
      if (hour == 0) hour = 12;
      if (!AppendNumber(output, capacity, &length, hour, 2, '0')) return 0;
    } else if (conversion == 'M') {
      if (!AppendNumber(output, capacity, &length,
                        value->tm_min, 2, '0')) return 0;
    } else if (conversion == 'S') {
      if (!AppendNumber(output, capacity, &length,
                        value->tm_sec, 2, '0')) return 0;
    } else if (conversion == 'j') {
      if (!AppendNumber(output, capacity, &length,
                        value->tm_yday + 1, 3, '0')) return 0;
    } else if (conversion == 'w') {
      if (!AppendNumber(output, capacity, &length,
                        value->tm_wday, 1, '0')) return 0;
    } else if (conversion == 'u') {
      int day = value->tm_wday == 0 ? 7 : value->tm_wday;
      if (!AppendNumber(output, capacity, &length, day, 1, '0')) return 0;
    } else if (conversion == 'a' || conversion == 'A') {
      text = value->tm_wday >= 0 && value->tm_wday < 7
                 ? week_days[value->tm_wday] : "?";
      if (conversion == 'a') {
        char short_name[4] = {text[0], text[1], text[2], '\0'};
        if (!AppendText(output, capacity, &length, short_name)) return 0;
      } else if (!AppendText(output, capacity, &length, text)) {
        return 0;
      }
    } else if (conversion == 'b' || conversion == 'B' || conversion == 'h') {
      text = value->tm_mon >= 0 && value->tm_mon < 12
                 ? months[value->tm_mon] : "?";
      if (conversion != 'B') {
        char short_name[4] = {text[0], text[1], text[2], '\0'};
        if (!AppendText(output, capacity, &length, short_name)) return 0;
      } else if (!AppendText(output, capacity, &length, text)) {
        return 0;
      }
    } else if (conversion == 'p') {
      if (!AppendText(output, capacity, &length,
                      value->tm_hour < 12 ? "AM" : "PM")) return 0;
    } else if (conversion == 'n' || conversion == 't') {
      if (length + 1 >= capacity) return 0;
      output[length++] = conversion == 'n' ? '\n' : '\t';
    } else if (conversion == 'z') {
      int offset = (int)value->tm_gmtoff;
      char sign = '+';
      if (offset < 0) {
        sign = '-';
        offset = -offset;
      }
      if (length + 1 >= capacity) return 0;
      output[length++] = sign;
      if (!AppendNumber(output, capacity, &length,
                        offset / 3600, 2, '0') ||
          !AppendNumber(output, capacity, &length,
                        (offset / 60) % 60, 2, '0')) return 0;
    } else if (conversion == 'Z') {
      if (!AppendText(output, capacity, &length,
                      value->tm_zone == NULL ? "" : value->tm_zone)) return 0;
    } else if (conversion == 'F' || conversion == 'D' ||
               conversion == 'R' || conversion == 'T') {
      const char* replacement =
          conversion == 'F' ? "%Y-%m-%d" :
          conversion == 'D' ? "%m/%d/%y" :
          conversion == 'R' ? "%H:%M" : "%H:%M:%S";
      char nested[32];
      size_t nested_length = strftime(nested, sizeof(nested),
                                      replacement, value);
      if (nested_length == 0 ||
          !AppendText(output, capacity, &length, nested)) return 0;
    } else {
      if (length + 2 >= capacity) return 0;
      output[length++] = '%';
      output[length++] = conversion;
    }
  }
  output[length] = '\0';
  return length;
}

static const char* ParseNumber(const char* input, int width, int* output) {
  int value = 0;
  int digits = 0;
  while (digits < width && input[digits] >= '0' && input[digits] <= '9') {
    value = value * 10 + input[digits] - '0';
    digits++;
  }
  if (digits == 0) return NULL;
  *output = value;
  return input + digits;
}

char* strptime(const char* restrict input, const char* restrict format,
               struct tm* restrict value) {
  while (*format != '\0') {
    int parsed;
    int width;
    char conversion;
    const char* next;
    if (*format != '%') {
      if (*input++ != *format++) return NULL;
      continue;
    }
    conversion = *++format;
    format++;
    if (conversion == '%') {
      if (*input++ != '%') return NULL;
      continue;
    }
    width = conversion == 'Y' ? 4 :
            conversion == 'j' ? 3 : 2;
    next = ParseNumber(input, width, &parsed);
    if (next == NULL) return NULL;
    if (conversion == 'Y') value->tm_year = parsed - 1900;
    else if (conversion == 'y') value->tm_year = parsed + (parsed < 69 ? 100 : 0);
    else if (conversion == 'm') value->tm_mon = parsed - 1;
    else if (conversion == 'd' || conversion == 'e') value->tm_mday = parsed;
    else if (conversion == 'H') value->tm_hour = parsed;
    else if (conversion == 'M') value->tm_min = parsed;
    else if (conversion == 'S') value->tm_sec = parsed;
    else if (conversion == 'j') value->tm_yday = parsed - 1;
    else return NULL;
    input = next;
  }
  return (char*)input;
}

char* tzname[] = {"UTC", "UTC"};

void tzset(void) {}
