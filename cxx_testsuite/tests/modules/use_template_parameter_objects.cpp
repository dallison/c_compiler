import template_parameter_objects;

int main() {
  if (module_sum<>() != 42 || module_sum<ModuleValue{7, 8}>() != 15) {
    return 1;
  }
  const ModuleValue* first =
      module_parameter_object<ModuleValue{20, 22}>();
  const ModuleValue* second =
      module_parameter_object<ModuleValue{20, 22}>();
  return first == second ? 0 : 2;
}
