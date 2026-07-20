long int long_value(void);
long long int long_long_value(void);
unsigned long int unsigned_long_value(void);
unsigned long long int unsigned_long_long_value(void);
signed int signed_value(void);
short int short_value(void);

long long_value(void) {
  return 1;
}

long long long_long_value(void) {
  return 2;
}

unsigned long unsigned_long_value(void) {
  return 3;
}

unsigned long long unsigned_long_long_value(void) {
  return 4;
}

signed signed_value(void) {
  return 5;
}

short short_value(void) {
  return 6;
}

int main(void) {
  return long_value() != 1 ||
         long_long_value() != 2 ||
         unsigned_long_value() != 3 ||
         unsigned_long_long_value() != 4 ||
         signed_value() != 5 ||
         short_value() != 6;
}
