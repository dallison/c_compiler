// RUN: -std=c++20

#include <bit>
#include <type_traits>

#if __cpp_lib_bit_cast != 201806L
#error "__cpp_lib_bit_cast has the wrong value"
#endif
#if __cpp_lib_bitops != 201907L
#error "__cpp_lib_bitops has the wrong value"
#endif
#if __cpp_lib_int_pow2 != 202002L
#error "__cpp_lib_int_pow2 has the wrong value"
#endif
#if __cpp_lib_endian != 201907L
#error "__cpp_lib_endian has the wrong value"
#endif

struct trivial_pair {
  unsigned short first;
  unsigned short second;
};

struct nontrivial {
  nontrivial(const nontrivial&) {
  }
  int value;
};

static_assert(std::is_trivially_copyable_v<int>);
static_assert(std::is_trivially_copyable_v<trivial_pair>);
static_assert(!std::is_trivially_copyable_v<int&>);
static_assert(!std::is_trivially_copyable_v<nontrivial>);

static_assert(std::__bit_detail::__digits<unsigned int> == 32);
static_assert(std::endian::native == std::endian::little);
static_assert(std::rotl<unsigned int>(0x12345678u, 8) == 0x34567812u);
static_assert(std::rotr<unsigned int>(0x12345678u, 8) == 0x78123456u);
static_assert(std::rotl<unsigned int>(0x12345678u, -8) == 0x78123456u);
static_assert(std::countl_zero<unsigned char>(0x10u) == 3);
static_assert(std::countl_one<unsigned char>(0xf0u) == 4);
static_assert(std::countr_zero<unsigned int>(0x30u) == 4);
static_assert(std::countr_one<unsigned int>(0x0fu) == 4);
static_assert(std::popcount<unsigned int>(0xf123u) == 8);
static_assert(std::has_single_bit<unsigned int>(0x8000u));
static_assert(!std::has_single_bit<unsigned int>(0x8001u));
static_assert(std::bit_floor<unsigned int>(31u) == 16u);
static_assert(std::bit_ceil<unsigned int>(31u) == 32u);
static_assert(std::bit_width<unsigned int>(31u) == 5);

constexpr unsigned int one_bits = std::bit_cast<unsigned int>(1.0f);
static_assert(one_bits == 0x3f800000u);

int main() {
  return 0;
}
