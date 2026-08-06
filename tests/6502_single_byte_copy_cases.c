typedef struct {
  unsigned char value;
} Byte;

typedef struct {
  unsigned char value[2];
} Pair;

void copy_byte(Byte* destination, const Byte* source) {
  *destination = *source;
}

void copy_byte_twice(Byte* destination, const Byte* source) {
  *destination = *source;
  *destination = *source;
}

void copy_pair(Pair* destination, const Pair* source) {
  *destination = *source;
}

void zero_byte(Byte* destination) {
  *destination = (Byte){0};
}

void zero_pair(Pair* destination) {
  *destination = (Pair){{0, 0}};
}
