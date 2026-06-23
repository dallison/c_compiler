// RUN: -std=c++20

struct ConstAggregate {
  const int value;
};

void fail_implicitly_deleted_assignment() {
  ConstAggregate left = {1};
  ConstAggregate right = {2};
  left = right;
}
