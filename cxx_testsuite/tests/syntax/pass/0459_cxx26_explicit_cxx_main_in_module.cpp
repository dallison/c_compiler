// RUN: -std=c++26

export module explicit_main;

int module_private_value = 0;

extern "C++" int main() {
  return module_private_value;
}
