// RUN: -std=c++20

#include <ranges>
#include <type_traits>

using Counted = std::counted_iterator<int*>;
using Common = std::common_iterator<Counted, std::default_sentinel_t>;
using ConstCommon =
    std::common_iterator<std::counted_iterator<const int*>,
                         std::default_sentinel_t>;

static_assert(std::input_or_output_iterator<Common>);
static_assert(std::input_iterator<Common>);
static_assert(std::forward_iterator<Common>);
static_assert(std::sentinel_for<Common, Common>);
static_assert(std::sized_sentinel_for<Common, Common>);
static_assert(std::same_as<std::iter_value_t<Common>, int>);
static_assert(std::same_as<std::iter_reference_t<Common>, int&>);
static_assert(std::same_as<std::iter_difference_t<Common>, long>);
static_assert(std::same_as<
              typename std::iterator_traits<Common>::iterator_concept,
              std::forward_iterator_tag>);
static_assert(std::is_constructible_v<ConstCommon, const Common&>);
static_assert(std::is_assignable_v<ConstCommon&, const Common&>);

using CountedRange =
    std::ranges::subrange<Counted, std::default_sentinel_t>;
using CommonView = std::ranges::common_view<CountedRange>;
static_assert(std::ranges::common_range<CommonView>);
static_assert(std::ranges::sized_range<CommonView>);
static_assert(std::ranges::common_range<const CommonView>);
static_assert(std::ranges::sized_range<const CommonView>);
static_assert(std::same_as<
              std::ranges::iterator_t<CommonView>,
              std::common_iterator<std::ranges::iterator_t<CountedRange>,
                                   std::ranges::sentinel_t<CountedRange>>>);

int main() {}
