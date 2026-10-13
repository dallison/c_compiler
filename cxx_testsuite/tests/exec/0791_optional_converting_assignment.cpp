#include <optional>
#include <string>

struct Value {
  Value(const std::string& value) : size(value.size()) {}

  int size;
};

int main() {
  std::optional<Value> value;
  std::string text = "abc";
  value = text;
  return !value || value->size != 3;
}
