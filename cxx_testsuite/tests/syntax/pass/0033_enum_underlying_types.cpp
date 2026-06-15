// RUN: -std=c++17
enum class ByteColor : unsigned char {
  red,
  green = 7,
};

enum Code : short {
  ok = 0,
  fail = -1,
};

enum class Forward : unsigned int;
enum class Forward : unsigned int {
  value = 10,
};

typedef unsigned char byte;
enum class AliasBased : byte {
  small = 1,
};

int main(void) {
  ByteColor color = ByteColor::green;
  Code code = fail;
  Forward forward = Forward::value;
  AliasBased alias = AliasBased::small;
  return sizeof(color) + sizeof(code) + sizeof(forward) + sizeof(alias);
}
