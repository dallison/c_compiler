import alignas_templates;

struct alignas(32) Wide {
  char c;
};

static_assert(alignof(PackAligned<char, Wide, int>) == 32);
static_assert(alignof(PackAligned<>) == 1);
static_assert(alignof(ValuePackMember<2, 16>) == 16);
static_assert(alignof(ValuePackMember<1, 2>) == 4);
static_assert(alignof(Outer<char>::Inner<Wide>) == 32);
static_assert(alignof(Outer<Wide>::Inner<char>) == 32);

int main() { return LocalIsAligned<Wide>() ? 0 : 1; }
