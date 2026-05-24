#ifndef LIBC_TEST_FRAMEWORK_H
#define LIBC_TEST_FRAMEWORK_H

#define CHECK(cond, failures)                                               \
  do {                                                                      \
    if (!(cond)) {                                                          \
      (failures)++;                                                         \
    }                                                                       \
  } while (0)

#define CHECK_EQ(actual, expected, failures)                                \
  do {                                                                      \
    long _actual_value = (long)(actual);                                    \
    long _expected_value = (long)(expected);                                \
    if (_actual_value != _expected_value) {                                 \
      (failures)++;                                                         \
    }                                                                       \
  } while (0)

#endif /* LIBC_TEST_FRAMEWORK_H */
