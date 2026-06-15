// RUN: -std=c++17
// EXPECT: guarded is a protected member of ProtectedBox
class ProtectedBox {
protected:
  int guarded;
public:
  int visible;
};

int main(void) {
  ProtectedBox box;
  return sizeof(box.guarded);
}
