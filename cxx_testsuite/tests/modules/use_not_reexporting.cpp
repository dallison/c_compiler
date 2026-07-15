import not_reexporting;

int main() {
  return dependency_wrapper() == 21 ? 0 : 1;
}
