module;
#include <meta>

export module token_injection;

export inline constexpr auto exported_tokens =
    ^{ constexpr int imported_injected = \(13); };

export namespace generated {
consteval {
  std::meta::queue_injection(
      ^{ constexpr int module_injected = \(29); });
}
}
