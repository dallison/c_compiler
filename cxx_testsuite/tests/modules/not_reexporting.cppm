export module not_reexporting;

import private_dependency;

export int dependency_wrapper() {
  return dependency_value() + 1;
}
