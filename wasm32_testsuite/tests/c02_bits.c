int main(void){ unsigned x=0xF0F0; x ^= 0x0FF0; x <<= 2; x >>= 3; return (x & 0xFF); }
