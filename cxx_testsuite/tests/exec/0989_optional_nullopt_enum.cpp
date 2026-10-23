// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <optional>

enum Semantic { kNone = 0, kSet = 1 };

struct Rec {
  template <typename Desc>
  Rec(const Desc* desc, std::optional<Semantic> semantic = std::nullopt)
      : semantic(semantic) {
    (void)desc;
  }
  std::optional<Semantic> semantic;
};

struct Desc {
  int value;
};

void take(Rec rec) {
  if (rec.semantic.has_value()) {
    rec.semantic = std::nullopt;
  }
}

int main() {
  Desc desc;
  desc.value = 1;
  // Passing a pointer copy-initializes Rec, which default-initializes its
  // optional parameter from nullopt.  That conversion is a separate
  // full-expression from the Rec constructor itself.
  take(&desc);
  return 0;
}
