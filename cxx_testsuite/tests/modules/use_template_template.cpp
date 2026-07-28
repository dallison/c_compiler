import template_template;

int main() {
  module_holder<module_box, int> holder{};
  holder.value.value = 42;
  return holder.value.value == 42 ? 0 : 1;
}
