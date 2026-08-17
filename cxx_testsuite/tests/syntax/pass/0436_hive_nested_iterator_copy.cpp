// RUN: -std=c++26

#include <hive>

void copy_hive_iterator(std::hive<int>& values) {
  std::hive<int>::iterator first = values.begin();
  std::hive<int>::iterator second = first;
  std::hive<int>::const_iterator constant = first;
  values.unique();
  (void)second;
  (void)constant;
}
