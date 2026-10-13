#include <variant>
#include <vector>

struct Info {
  struct NonField {
    int function;
    unsigned tag;
    unsigned data;
  };

  std::variant<int, NonField> value;
};

int main() {
  std::vector<Info> fields;
  fields = {{Info::NonField{7, 0, 0}}};
  const Info::NonField& value = std::get<Info::NonField>(fields[0].value);
  return value.function != 7;
}
