int main(void){ unsigned long long v=0x123456789ABCDEFull; v>>=13; v^=0xFFFF; return (int)(v&0x7F); }
