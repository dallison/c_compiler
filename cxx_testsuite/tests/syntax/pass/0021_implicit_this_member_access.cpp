// RUN: -std=c++17
class HasFields {
public:
  int value;
  int size_of_value(void);
};

int HasFields::size_of_value(void) {
  return sizeof(value);
}

int main(void) {
  return 0;
}
