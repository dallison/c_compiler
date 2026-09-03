#include "chrono_host.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect_true(int condition, const char* message) {
  if (!condition) {
    fprintf(stderr, "chrono_host_test failed: %s\n", message);
    return 1;
  }
  return 0;
}

static int check_sys_info(const char* zone, int64_t sys_seconds,
                          int32_t expected_offset, const char* expected_abbrev) {
  DaveHostChronoSysInfoWire info;
  char abbrev[16];
  DaveHostChronoSysInfoRequestWire request = {
      (uint64_t)(uintptr_t)&sys_seconds, (uint64_t)(uintptr_t)&info,
      (uint64_t)(uintptr_t)abbrev, sizeof(abbrev), 0};
  int64_t result = DaveHostChronoSysInfoRequest(zone, &request);
  if (result != 0) {
    fprintf(stderr, "sys_info %s failed with %lld\n", zone, (long long)result);
    return 1;
  }
  if (info.offset_seconds != expected_offset) {
    fprintf(stderr, "sys_info %s offset got %d want %d\n", zone,
            info.offset_seconds, expected_offset);
    return 1;
  }
  if (strcmp(abbrev, expected_abbrev) != 0) {
    fprintf(stderr, "sys_info %s abbrev got %s want %s\n", zone, abbrev,
            expected_abbrev);
    return 1;
  }
  return 0;
}

int main(void) {
  const char* fixture_root = getenv("DAVE_TZDIR");
  if (fixture_root == NULL) {
    fixture_root = "tests/fixtures/tzif";
  }
  setenv("DAVE_TZDIR", fixture_root, 1);
  setenv("TZ", "FixedOffset", 1);

  char version[64];
  char current[64];
  char canonical[64];
  char abbrev[32];
  uint32_t zone_count = 0;
  uint32_t zone_index = 0;
  uint64_t generation = 0;
  uint32_t leap_count = 0;
  DaveHostChronoSysInfoWire sys_info;
  DaveHostChronoLocalInfoWire local_info;
  DaveHostChronoLeapSecond leap_info;

  if (expect_true(DaveHostChronoTzdbVersion(version, sizeof(version)) == 0,
                  "version") ||
      expect_true(version[0] != '\0', "non-empty version") ||
      expect_true(DaveHostChronoZoneCount(&zone_count) == 0, "zone count") ||
      expect_true(zone_count >= 4, "fixture zones present") ||
      expect_true(DaveHostChronoLocateZone("Alias", canonical, sizeof(canonical),
                                           &zone_index) == 0,
                  "resolve alias") ||
      expect_true(strcmp(canonical, "FixedOffset") == 0, "alias canonical") ||
      expect_true(check_sys_info("FixedOffset", 100, 3600, "FIX") == 0,
                  "fixed offset sys info") ||
      expect_true(check_sys_info("DST", 1700000000, 0, "STD") == 0, "dst winter") ||
      expect_true(check_sys_info("DST", 1712000000, 3600, "DST") == 0, "dst summer")) {
    return 1;
  }

  int64_t gap_local = 1711844000;
  DaveHostChronoLocalInfoRequestWire gap_request = {
      (uint64_t)(uintptr_t)&gap_local, (uint64_t)(uintptr_t)&local_info,
      (uint64_t)(uintptr_t)abbrev, 16, 0};
  if (DaveHostChronoLocalInfoRequest("DST", &gap_request) != 0 ||
      local_info.result != DAVE_HOST_CHRONO_LOCAL_NONEXISTENT) {
    fprintf(stderr, "chrono_host_test failed: dst gap\n");
    return 1;
  }

  int64_t fold_local = 1730593800;
  DaveHostChronoLocalInfoRequestWire fold_request = {
      (uint64_t)(uintptr_t)&fold_local, (uint64_t)(uintptr_t)&local_info,
      (uint64_t)(uintptr_t)abbrev, 16, 0};
  if (DaveHostChronoLocalInfoRequest("DST", &fold_request) != 0 ||
      local_info.result != DAVE_HOST_CHRONO_LOCAL_AMBIGUOUS) {
    fprintf(stderr, "chrono_host_test failed: dst fold\n");
    return 1;
  }

  if (DaveHostChronoGeneration(&generation) != 0 || generation == 0 ||
      DaveHostChronoReload(&generation) != 0 || generation <= 1 ||
      DaveHostChronoCurrentZone(current, sizeof(current)) != 0 ||
      strcmp(current, "FixedOffset") != 0 ||
      DaveHostChronoLeapCount(&leap_count) != 0 || leap_count != 2 ||
      DaveHostChronoLeapInfo(1, &leap_info) != 0 ||
      leap_info.correction_seconds != 2) {
    fprintf(stderr, "chrono_host_test failed: reload/current/leap\n");
    return 1;
  }

  int64_t missing_seconds = 0;
  DaveHostChronoSysInfoRequestWire missing_request = {
      (uint64_t)(uintptr_t)&missing_seconds, (uint64_t)(uintptr_t)&sys_info,
      (uint64_t)(uintptr_t)abbrev, sizeof(abbrev), 0};
  if (DaveHostChronoSysInfoRequest("MissingZone", &missing_request) == 0 ||
      DaveHostChronoLocateZone("MissingZone", canonical, sizeof(canonical),
                               &zone_index) == 0) {
    fprintf(stderr, "chrono_host_test failed: missing zone\n");
    return 1;
  }

  setenv("DAVE_TZDIR", "/nonexistent/dave-tzdir", 1);
  DaveHostChronoReload(NULL);
  if (DaveHostChronoCurrentZone(current, sizeof(current)) != 0 ||
      strcmp(current, "UTC") != 0) {
    fprintf(stderr, "chrono_host_test failed: utc fallback\n");
    return 1;
  }
  DaveHostChronoSysInfoRequestWire utc_request = {
      (uint64_t)(uintptr_t)&missing_seconds, (uint64_t)(uintptr_t)&sys_info,
      (uint64_t)(uintptr_t)abbrev, sizeof(abbrev), 0};
  if (DaveHostChronoSysInfoRequest("UTC", &utc_request) != 0 ||
      sys_info.offset_seconds != 0) {
    fprintf(stderr, "chrono_host_test failed: utc fallback sys info\n");
    return 1;
  }
  if (DaveHostChronoLocateZone("Europe/Paris", canonical, sizeof(canonical),
                               &zone_index) == 0) {
    fprintf(stderr, "chrono_host_test failed: utc fallback missing zone\n");
    return 1;
  }

  unsetenv("DAVE_TZDIR");

  return 0;
}
