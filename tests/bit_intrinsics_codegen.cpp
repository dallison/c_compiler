unsigned int rotate_left(unsigned int value, int amount) {
  return __davecc_rotl(value, amount);
}

unsigned int rotate_right(unsigned int value, int amount) {
  return __davecc_rotr(value, amount);
}

int leading_zeroes(unsigned int value) {
  return __davecc_clz(value, sizeof(value) * 8);
}

int trailing_zeroes(unsigned int value) {
  return __davecc_ctz(value, sizeof(value) * 8);
}

int population_count(unsigned int value) {
  return __davecc_popcount(value, sizeof(value) * 8);
}
