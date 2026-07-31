// RUN: -std=c++23

#include <ranges>
#include <type_traits>

void check_window_views() {
  int values[] = {1, 2, 3, 4, 5, 6};

  auto chunks = std::views::chunk(values, 2);
  static_assert(std::ranges::view<decltype(chunks)>);
  static_assert(std::ranges::forward_range<decltype(chunks)>);
  static_assert(std::ranges::sized_range<decltype(chunks)>);
  static_assert(std::ranges::range<
                std::ranges::range_reference_t<decltype(chunks)>>);

  auto piped_chunks = values | std::views::chunk(3);
  static_assert(std::ranges::view<decltype(piped_chunks)>);

  auto slides = std::views::slide(values, 3);
  static_assert(std::ranges::view<decltype(slides)>);
  static_assert(std::ranges::forward_range<decltype(slides)>);
  static_assert(std::ranges::sized_range<decltype(slides)>);
  static_assert(std::ranges::range<
                std::ranges::range_reference_t<decltype(slides)>>);

  auto piped_slides = values | std::views::slide(2);
  static_assert(std::ranges::view<decltype(piped_slides)>);

  auto increasing = [](int left, int right) {
    return left < right;
  };
  auto grouped = std::views::chunk_by(values, increasing);
  static_assert(std::ranges::view<decltype(grouped)>);
  static_assert(std::ranges::forward_range<decltype(grouped)>);
  auto grouped_chunk = *grouped.begin();
  auto grouped_first = grouped_chunk.begin();
  (void)grouped_first;

  auto piped_grouped = values | std::views::chunk_by(increasing);
  static_assert(std::ranges::view<decltype(piped_grouped)>);

  auto strided = std::views::stride(values, 2);
  static_assert(std::ranges::view<decltype(strided)>);
  static_assert(std::ranges::forward_range<decltype(strided)>);
  static_assert(std::ranges::sized_range<decltype(strided)>);
  static_assert(std::same_as<
                std::ranges::range_reference_t<decltype(strided)>, int&>);

  auto piped_strided = values | std::views::stride(3);
  static_assert(std::ranges::view<decltype(piped_strided)>);

  auto counted = std::views::counted(values, 6);
  static_assert(!std::ranges::common_range<decltype(counted)>);
  auto counted_chunks = counted | std::views::chunk(2);
  auto counted_slides = counted | std::views::slide(2);
  auto counted_grouped = counted | std::views::chunk_by(increasing);
  auto counted_strided = counted | std::views::stride(2);
  static_assert(std::ranges::view<decltype(counted_chunks)>);
  static_assert(std::ranges::view<decltype(counted_slides)>);
  static_assert(std::ranges::view<decltype(counted_grouped)>);
  static_assert(std::ranges::view<decltype(counted_strided)>);
}
