// RUN: -std=c++26

void uses_block() {
  consteval {
    ;
  }
}
