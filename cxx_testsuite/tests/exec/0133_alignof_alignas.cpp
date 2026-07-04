// RUN: -std=c++20
// EXPECT_EXIT: 0

alignas(32) int global_aligned = 7;

struct alignas(16) AlignedStruct {
  char c;
};

struct MemberAligned {
  char first;
  alignas(8) int value;
  char last;
};

struct TypeAlignedMember {
  char first;
  alignas(long long) char value;
};

int main(void) {
  if (alignof(char) != 1) {
    return 1;
  }
  if (alignof(long long) < alignof(int)) {
    return 2;
  }
  if (alignof(AlignedStruct) != 16 || sizeof(AlignedStruct) != 16) {
    return 3;
  }

  MemberAligned member_aligned;
  int value_offset =
      (int)((char*)&member_aligned.value - (char*)&member_aligned);
  if (value_offset != 8) {
    return 4;
  }
  if (alignof(MemberAligned) != 8 || sizeof(MemberAligned) != 16) {
    return 5;
  }

  TypeAlignedMember type_aligned;
  int type_value_offset =
      (int)((char*)&type_aligned.value - (char*)&type_aligned);
  if (type_value_offset != alignof(long long)) {
    return 6;
  }

  if (global_aligned != 7) {
    return 7;
  }

  int ordinary = 0;
  if (alignof(ordinary) != alignof(int)) {
    return 8;
  }

  return 0;
}
