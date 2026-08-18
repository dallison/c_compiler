export module erroneous_value_state;

export inline int module_erroneous_zero_policy() {
  int value;
  return value;
}

export inline bool module_indeterminate_attribute() {
  int value [[indeterminate]];
  return &value != nullptr;
}
