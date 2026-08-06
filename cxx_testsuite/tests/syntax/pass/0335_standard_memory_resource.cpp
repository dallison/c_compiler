// RUN: -std=c++20

#include <deque>
#include <list>
#include <map>
#include <memory_resource>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if __cpp_lib_memory_resource != 201603L
#error "__cpp_lib_memory_resource has the wrong value"
#endif

static_assert(sizeof(std::pmr::polymorphic_allocator<int>) == sizeof(void*));

int main() {
  std::pmr::vector<int> values;
  std::pmr::deque<int> deque_values;
  std::pmr::list<int> list_values;
  std::pmr::map<int, long> map_values;
  std::pmr::set<int> set_values;
  std::pmr::unordered_map<int, long> unordered_map_values;
  std::pmr::unordered_set<int> unordered_set_values;
  std::pmr::string text;
  (void)values;
  (void)deque_values;
  (void)list_values;
  (void)map_values;
  (void)set_values;
  (void)unordered_map_values;
  (void)unordered_set_values;
  (void)text;
  return 0;
}
