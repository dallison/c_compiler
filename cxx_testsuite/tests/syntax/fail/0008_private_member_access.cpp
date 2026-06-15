// RUN: -std=c++17
// EXPECT: hidden is a private member of PrivateBox
class PrivateBox {
  int hidden;
public:
  int visible;
};

int main(void) {
  PrivateBox box;
  return sizeof(box.hidden);
}
