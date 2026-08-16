export module meta_synthesis;

namespace meta_synth {
using info = decltype(^^::);
[[davecc::meta_intrinsic]] consteval info reflect_constant(auto value);
}

export using info = meta_synth::info;
export constexpr auto imported_value = meta_synth::reflect_constant(17);
