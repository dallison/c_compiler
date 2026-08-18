import erroneous_value_state;

int main() {
  if (module_erroneous_zero_policy() != 0) {
    return 1;
  }
  return module_indeterminate_attribute() ? 0 : 2;
}
