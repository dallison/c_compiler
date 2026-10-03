// RUN: -std=c++20
// EXPECT_EXIT: 0

// A const local initialized by a constexpr member call on the current object
// reads that object's runtime state; it is not a constant expression
// (std::chrono::weekday::operator[] lost the weekday this way).

struct Packed {
  unsigned short data;
  Packed(unsigned low, unsigned high)
      : data(static_cast<unsigned short>((high << 8) | low)) {}
};

struct Day {
  unsigned char value;
  constexpr explicit Day(unsigned v) : value(static_cast<unsigned char>(v)) {}
  constexpr explicit operator unsigned() const noexcept { return value; }
  constexpr unsigned get() const { return value; }
  unsigned via_get() const;
  unsigned via_this_get() const;
  Packed via_conversion(unsigned index) const;
};

unsigned Day::via_get() const {
  const unsigned v = get();
  return v;
}

unsigned Day::via_this_get() const {
  const unsigned v = this->get();
  return v;
}

Packed Day::via_conversion(unsigned index) const {
  const unsigned v = static_cast<unsigned>(*this);
  return Packed(v, index);
}

int main() {
  const Day day(4);
  if (day.via_get() != 4) return 1;
  if (day.via_this_get() != 4) return 2;
  if (day.via_conversion(3).data != 0x304) return 3;
  return 0;
}
