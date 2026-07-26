template <typename T>
struct Buffer {
  Buffer()
      : first(nullptr),
        second(nullptr),
        third(nullptr),
        fourth(nullptr),
        fifth(nullptr),
        sixth(nullptr) {}

  static constexpr int eof() { return -1; }
  virtual int overflow(int = -1) { return eof(); }

  T* first;
  T* second;
  T* third;
  T* fourth;
  T* fifth;
  T* sixth;
};

Buffer<unsigned char> buffer;

extern unsigned char first_symbol[];
extern unsigned char last_symbol[];
extern void consume_symbols(unsigned char*, unsigned char*);

void push_symbol_addresses() {
  consume_symbols(first_symbol, last_symbol);
}

unsigned short widen_indirect(const unsigned char* value) {
  unsigned char byte = *value;
  return byte;
}

unsigned short widen_bool(const unsigned char* value) {
  return *value != 0;
}

extern unsigned char produce_byte();

unsigned short widen_call_result() {
  unsigned char byte = produce_byte();
  return byte;
}
