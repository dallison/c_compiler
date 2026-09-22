// RUN: -std=c++17
// EXPECT_EXIT: 0

enum class Flags : unsigned char { kSign = 1 };

class ConvTag {
 public:
  constexpr ConvTag(Flags flags)
      : tag_(static_cast<unsigned char>(0xc0 | static_cast<unsigned char>(flags))) {}
  constexpr ConvTag() : tag_(0xFF) {}
  constexpr unsigned char get() const { return tag_; }

 private:
  unsigned char tag_;
};

struct Holder {
  static constexpr auto kFSign = Flags::kSign;
  static constexpr ConvTag value[4] = {{}, kFSign, {}, {}};
};

int main() {
  return Holder::value[0].get() == 0xFF &&
                 Holder::value[1].get() ==
                     static_cast<unsigned char>(0xc0 | 1) &&
                 Holder::value[2].get() == 0xFF
             ? 0
             : 1;
}
