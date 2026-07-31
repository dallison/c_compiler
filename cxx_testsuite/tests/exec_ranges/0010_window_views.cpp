// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <ranges>

int main() {
  int values[] = {1, 2, 3, 4, 5};

  auto chunks = values | std::views::chunk(2);
  if (chunks.size() != 3) {
    return 1;
  }
  int chunk_count = 0;
  int total = 0;
  int last_size = 0;
  for (auto chunk : chunks) {
    last_size = 0;
    for (int value : chunk) {
      total += value;
      ++last_size;
    }
    ++chunk_count;
  }
  if (chunk_count != 3 || total != 15 || last_size != 1) {
    return 2;
  }

  int short_values[] = {1, 2, 3};
  auto one_chunk = std::views::chunk(short_values, 4);
  chunk_count = 0;
  total = 0;
  for (auto chunk : one_chunk) {
    for (int value : chunk) {
      total += value;
    }
    ++chunk_count;
  }
  if (one_chunk.size() != 1 || chunk_count != 1 || total != 6) {
    return 3;
  }

  auto slides = std::views::slide(values, 2);
  if (slides.size() != 4) {
    return 4;
  }
  int window_count = 0;
  total = 0;
  for (auto window : slides) {
    for (int value : window) {
      total += value;
    }
    ++window_count;
  }
  if (window_count != 4 || total != 24) {
    return 5;
  }

  auto empty_slides = short_values | std::views::slide(4);
  if (empty_slides.size() != 0 ||
      empty_slides.begin() != empty_slides.end()) {
    return 6;
  }

  int mutable_values[] = {1, 2, 3, 4};
  for (auto chunk : mutable_values | std::views::chunk(2)) {
    for (int& value : chunk) {
      value += 10;
    }
  }
  if (mutable_values[0] != 11 || mutable_values[1] != 12 ||
      mutable_values[2] != 13 || mutable_values[3] != 14) {
    return 7;
  }

  int runs[] = {1, 2, 3, 1, 2, 5, 4};
  auto increasing = [](int left, int right) {
    return left < right;
  };
  auto groups = runs | std::views::chunk_by(increasing);
  int expected_sizes[] = {3, 3, 1};
  int group_count = 0;
  total = 0;
  for (auto group : groups) {
    int group_size = 0;
    for (int value : group) {
      total += value;
      ++group_size;
    }
    if (group_size != expected_sizes[group_count]) {
      return 8;
    }
    ++group_count;
  }
  if (group_count != 3 || total != 18) {
    return 9;
  }

  int one[] = {7};
  auto one_group = std::views::chunk_by(one, increasing);
  group_count = 0;
  for (auto group : one_group) {
    int group_size = 0;
    for (int value : group) {
      total += value;
      ++group_size;
    }
    if (group_size != 1) {
      return 10;
    }
    ++group_count;
  }
  if (group_count != 1) {
    return 11;
  }

  auto every_second = values | std::views::stride(2);
  if (every_second.size() != 3) {
    return 12;
  }
  total = 0;
  for (int value : every_second) {
    total += value;
  }
  if (total != 9) {
    return 13;
  }

  auto one_value = std::views::stride(values, 8);
  int stride_count = 0;
  total = 0;
  for (int value : one_value) {
    total += value;
    ++stride_count;
  }
  if (one_value.size() != 1 || stride_count != 1 || total != 1) {
    return 14;
  }

  auto counted = std::views::counted(values, 5);
  auto counted_chunks = counted | std::views::chunk(2);
  chunk_count = 0;
  for (auto chunk : counted_chunks) {
    ++chunk_count;
    (void)chunk;
  }
  if (chunk_count != 3) {
    return 15;
  }

  auto counted_slides = counted | std::views::slide(3);
  window_count = 0;
  for (auto window : counted_slides) {
    ++window_count;
    (void)window;
  }
  if (window_count != 3) {
    return 16;
  }

  auto counted_groups = counted | std::views::chunk_by(increasing);
  group_count = 0;
  for (auto group : counted_groups) {
    ++group_count;
    (void)group;
  }
  if (group_count != 1) {
    return 17;
  }

  auto counted_stride = counted | std::views::stride(2);
  stride_count = 0;
  total = 0;
  for (int value : counted_stride) {
    total += value;
    ++stride_count;
  }
  if (stride_count != 3 || total != 9) {
    return 18;
  }

  return 0;
}
