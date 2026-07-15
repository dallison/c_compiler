#ifndef DAVECC_TEST_HEADER_UNIT_HPP
#define DAVECC_TEST_HEADER_UNIT_HPP

#define HEADER_UNIT_VALUE 21
#define HEADER_UNIT_SCALE(x) ((x) * 2)

struct HeaderUnitBox {
  int value;
};

static int header_unit_internal_value() {
  return 20;
}

inline int header_unit_inline_value() {
  return header_unit_internal_value() + 1;
}

#endif
