export module invalid_internal_reachability;

static int internal_helper(int value) {
  return value + 1;
}

export template <typename T>
int exposes_internal(T value) {
  return internal_helper((int)value);
}
