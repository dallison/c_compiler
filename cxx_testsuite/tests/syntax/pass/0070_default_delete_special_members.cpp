// RUN: -std=c++20

struct DefaultedBox {
  int value;

  DefaultedBox() = default;
  DefaultedBox(const DefaultedBox& other) = default;
  DefaultedBox(DefaultedBox&& other) = default;
  struct DefaultedBox& operator=(const DefaultedBox& other) = default;
  struct DefaultedBox& operator=(DefaultedBox&& other) = default;
  ~DefaultedBox() = default;
};

struct DeletedBox {
  int value;

  DeletedBox() = default;
  DeletedBox(const DeletedBox& other) = delete;
  struct DeletedBox& operator=(const DeletedBox& other) = delete;
};

struct OutOfClassDefaulted {
  int value;

  OutOfClassDefaulted();
  ~OutOfClassDefaulted();
};

OutOfClassDefaulted::OutOfClassDefaulted() = default;
OutOfClassDefaulted::~OutOfClassDefaulted() = default;

int main(void) {
  DefaultedBox first;
  DefaultedBox second(first);
  DefaultedBox third((DefaultedBox&&)second);
  first = third;
  first = (DefaultedBox&&)third;

  DeletedBox deleted;
  OutOfClassDefaulted out_of_class;
  (void)deleted;
  (void)out_of_class;
  return 0;
}
