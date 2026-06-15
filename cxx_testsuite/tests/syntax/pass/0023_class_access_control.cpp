// RUN: -std=c++17
class AccessBox {
  int hidden;
  int secret(void);
protected:
  int guarded;
public:
  int visible;
  int expose(void);
  int call_secret(void);
};

int AccessBox::secret(void) {
  return hidden + guarded;
}

int AccessBox::expose(void) {
  return hidden + guarded + visible;
}

int AccessBox::call_secret(void) {
  return sizeof(this->secret());
}

int main(void) {
  AccessBox box;
  return sizeof(box.visible) + sizeof(box.expose());
}
