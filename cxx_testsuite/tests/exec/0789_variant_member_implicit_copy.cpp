#include <functional>
#include <string>
#include <variant>

struct Value {
  std::variant<std::string, std::function<bool()>> data;
};

int main() {
  Value first;
  first.data = std::string("ok");
  Value second = first;
  return std::get<std::string>(second.data) != "ok";
}
