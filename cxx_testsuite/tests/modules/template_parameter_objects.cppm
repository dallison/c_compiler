export module template_parameter_objects;

export struct ModuleValue {
  int first;
  int second;
};

export template <ModuleValue object = ModuleValue{20, 22}>
int module_sum() {
  return object.first + object.second;
}

export template <ModuleValue object>
const ModuleValue* module_parameter_object() {
  return &object;
}
