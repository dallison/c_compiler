export module invalid_private_reachability;

int private_helper(int value);

export template <typename T>
int exposes_private(T value) {
  return private_helper((int)value);
}

module :private;

int private_helper(int value) {
  return value + 1;
}
