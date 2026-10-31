// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <flat_map>
#include <string>

int main() {
  std::flat_map<std::string, std::string> values;
  values["keep"] = "fresh";
  values[values["keep"]];
  if (values.size() != 2) return 1;
  if (values.find("fresh") == values.end()) return 2;
  if (values["keep"] != "fresh") return 3;
  if (values["fresh"] != "") return 4;

  std::flat_map<std::string, std::string>::iterator keep = values.find("keep");
  values[keep->first];
  if (values.size() != 2 || values["keep"] != "fresh") return 5;
  return 0;
}
