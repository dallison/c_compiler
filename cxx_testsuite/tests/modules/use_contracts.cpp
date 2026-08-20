import contracts;

int main() {
  return module_checked(40) == 42 && module_virtual_checked(4) == 5 ? 0 : 1;
}
