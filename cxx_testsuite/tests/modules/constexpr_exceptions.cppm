export module constexpr_exceptions;

export struct module_exception {
  int value;
};

export constexpr int module_exception_value(int value) {
  try {
    if (value != 0) {
      throw module_exception{value};
    }
  } catch (const module_exception& error) {
    return error.value;
  }
  return 0;
}
