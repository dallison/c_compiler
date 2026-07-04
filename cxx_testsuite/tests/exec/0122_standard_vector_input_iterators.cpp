// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <iterator>
#include <vector>

struct SinglePassState {
  int* data;
  int size;
  int index;
};

struct SinglePassIterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = int;
  using difference_type = long;
  using pointer = int*;
  using reference = int&;

  SinglePassState* state;
  int end_index;
  bool is_end;

  SinglePassIterator() : state(nullptr), end_index(0), is_end(true) {}

  SinglePassIterator(SinglePassState* s, bool end)
      : state(s), end_index(s->size), is_end(end) {
  }

  int operator*() const {
    return state->data[state->index];
  }

  SinglePassIterator& operator++() {
    state->index = state->index + 1;
    if (state->index >= end_index) {
      is_end = true;
    }
    return *this;
  }

  SinglePassIterator operator++(int) {
    SinglePassIterator copy = *this;
    ++(*this);
    return copy;
  }
};

bool operator==(const SinglePassIterator& left,
                const SinglePassIterator& right) {
  if (left.is_end && right.is_end) {
    return true;
  }
  if (left.is_end != right.is_end) {
    return false;
  }
  return left.state == right.state && left.state->index == right.state->index;
}

bool operator!=(const SinglePassIterator& left,
                const SinglePassIterator& right) {
  return !(left == right);
}

SinglePassIterator single_pass_begin(SinglePassState* state) {
  state->index = 0;
  SinglePassIterator it;
  it.state = state;
  it.end_index = state->size;
  it.is_end = state->size == 0;
  return it;
}

SinglePassIterator single_pass_end(SinglePassState* state) {
  SinglePassIterator it;
  it.state = state;
  it.end_index = state->size;
  it.is_end = true;
  return it;
}

int main(void) {
  int initial[3] = {1, 2, 3};
  SinglePassState init_state = {initial, 3, 0};
  std::vector<int> values(single_pass_begin(&init_state),
                          single_pass_end(&init_state));
  if (values.size() != 3 || values[0] != 1 || values[2] != 3) {
    return 1;
  }

  int assigned[4] = {4, 5, 6, 7};
  SinglePassState assign_state = {assigned, 4, 0};
  values.assign(single_pass_begin(&assign_state),
                single_pass_end(&assign_state));
  if (values.size() != 4 || values[0] != 4 || values[3] != 7) {
    return 2;
  }

  int inserted[2] = {8, 9};
  SinglePassState insert_state = {inserted, 2, 0};
  std::vector<int>::iterator pos =
      values.insert(values.begin() + 2, single_pass_begin(&insert_state),
                    single_pass_end(&insert_state));
  if (pos != values.begin() + 2) {
    return 3;
  }
  if (values.size() != 6 || values[0] != 4 || values[1] != 5 ||
      values[2] != 8 || values[3] != 9 || values[5] != 7) {
    return 4;
  }

  return 0;
}
