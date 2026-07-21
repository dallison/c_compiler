// RUN: -target 65c02

#include <any>
#include <array>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>

int use_no_exception_library(
    std::any& a, std::array<int, 1>& ar, std::deque<int>& d,
    std::function<int()>& f, std::map<int, int>& m, std::optional<int>& o,
    std::string_view view, std::unordered_map<int, int>& unordered,
    std::variant<int, long>& variant) {
  return std::any_cast<int>(a) + ar.at(0) + d.at(0) + f() + m.at(0) +
         o.value() + view.at(0) + unordered.at(0) + std::get<0>(variant);
}
