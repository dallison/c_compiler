import token_injection;

namespace std::meta {
using info = decltype(^^::);
[[davecc::meta_intrinsic]] consteval void namespace_inject(info namespace_,
                                                           info tokens);
}

namespace local {}

consteval void install_imported_tokens() {
  std::meta::namespace_inject(^^local, exported_tokens);
}

int main() {
  consteval { install_imported_tokens(); }
  if (generated::module_injected != 29) {
    return 1;
  }
  return local::imported_injected != 13;
}
