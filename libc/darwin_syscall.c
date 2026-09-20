// Darwin syscall() covers the guest TZDB / random numbers that chrono.cc
// and <random> still issue.  Filesystem goes through __davecc_linux_fs_service.

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <syscall.h>

#include "chrono_host.h"

int getentropy(void* buffer, size_t length);

long syscall(int number, ...) {
  va_list arguments;
  va_start(arguments, number);
  long a0 = va_arg(arguments, long);
  long a1 = va_arg(arguments, long);
  long a2 = va_arg(arguments, long);
  long a3 = va_arg(arguments, long);
  va_end(arguments);

  switch (number) {
    case SYS_TZDB_VERSION:
      return (long)DaveHostChronoTzdbVersion((char*)a0, (size_t)a1);
    case SYS_TZDB_GENERATION:
      return a0 == 0 ? -EINVAL : (long)DaveHostChronoGeneration((uint64_t*)a0);
    case SYS_TZDB_RELOAD:
      return (long)DaveHostChronoReload((uint64_t*)a0);
    case SYS_TZDB_CURRENT_ZONE:
      return (long)DaveHostChronoCurrentZone((char*)a0, (size_t)a1);
    case SYS_TZDB_ZONE_COUNT:
      return a0 == 0 ? -EINVAL : (long)DaveHostChronoZoneCount((uint32_t*)a0);
    case SYS_TZDB_ZONE_NAME:
      return (long)DaveHostChronoZoneName((uint32_t)a0, (char*)a1, (size_t)a2);
    case SYS_TZDB_LOCATE_ZONE:
      return a2 == 0 || a3 == 0
                 ? -EINVAL
                 : (long)DaveHostChronoLocateZone(
                       (const char*)a0, (char*)a1, (size_t)a2, (uint32_t*)a3);
    case SYS_TZDB_SYS_INFO:
      return a0 == 0 || a1 == 0
                 ? -EINVAL
                 : (long)DaveHostChronoSysInfoRequest(
                       (const char*)a0,
                       (const DaveHostChronoSysInfoRequestWire*)a1);
    case SYS_TZDB_LOCAL_INFO:
      return a0 == 0 || a1 == 0
                 ? -EINVAL
                 : (long)DaveHostChronoLocalInfoRequest(
                       (const char*)a0,
                       (const DaveHostChronoLocalInfoRequestWire*)a1);
    case SYS_TZDB_LEAP_COUNT:
      return a0 == 0 ? -EINVAL : (long)DaveHostChronoLeapCount((uint32_t*)a0);
    case SYS_TZDB_LEAP_INFO:
      return a1 == 0 ? -EINVAL
                     : (long)DaveHostChronoLeapInfo(
                           (uint32_t)a0, (DaveHostChronoLeapSecond*)a1);
    case SYS_RANDOM_BYTES:
      return getentropy((void*)a0, (size_t)a1);
    default:
      return -ENOSYS;
  }
}
