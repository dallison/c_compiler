// RUN: -std=c++26

int global_value;

void direct_parameter(int value)
    pre ((++value, true)) {}

void spliced_parameter(int value)
    pre ((++[:^^value:], true)) {}

void direct_global()
    pre ((++global_value, true)) {}

void spliced_global()
    pre ((++[:^^global_value:], true)) {}

void assertion_statement(int value) {
  contract_assert ((++value, true));
}

void assertion_structured_binding() {
  int pair[2] = {};
  auto& [first, second] = pair;
  contract_assert ((++[:^^first:], true));
}

void lambda_capture(int value)
    pre ([&] {
      ++value;
      return true;
    }()) {}

void spliced_lambda_capture(int value)
    pre ([&] {
      ++[:^^value:];
      return true;
    }()) {}
