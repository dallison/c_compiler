// RUN: -std=c++17

struct Box {
  int value;
  Box() : value(4) {
  }
};

int goto_over_closed_scope(int pick) {
  if (pick) {
    goto target;
  }
  {
    Box box;
    return box.value;
  }
target:
  return 0;
}

int goto_after_vacuous_scalar_declaration(int pick) {
  if (pick) {
    goto target;
  }
  int value;
target:
  value = 5;
  return value;
}

int switch_constructs_inside_case(int pick) {
  switch (pick) {
    case 1: {
      Box box;
      return box.value;
    }
    default:
      return 0;
  }
}

int switch_bypasses_vacuous_scalar_declaration(int pick) {
  switch (pick) {
    int value;
    case 1:
      value = 6;
      return value;
    default:
      return 0;
  }
}
