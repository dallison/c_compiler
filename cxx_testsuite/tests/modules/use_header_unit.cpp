import "header_unit.hpp";

#ifndef HEADER_UNIT_VALUE
#error "header-unit object-like macro was not imported"
#endif

#ifndef HEADER_UNIT_SCALE
#error "header-unit function-like macro was not imported"
#endif

int main() {
  HeaderUnitBox box{header_unit_inline_value()};
  return HEADER_UNIT_SCALE(box.value) == 42 ? 0 : 1;
}
