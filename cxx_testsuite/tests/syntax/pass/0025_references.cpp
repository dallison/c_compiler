// RUN: -std=c++17
int read_ref(int &value) {
  return value;
}

int write_ref(int &value) {
  value = 3;
  return value;
}

int read_rvalue_ref(int &&value) {
  return value;
}

int main(void) {
  int value;
  int &ref = value;
  return sizeof(ref) + sizeof(read_ref(value)) + sizeof(write_ref(ref));
}
