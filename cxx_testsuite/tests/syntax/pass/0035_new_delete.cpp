// RUN: -std=c++17
struct Box {
  Box();
  ~Box();
  int value;
};

unsigned long global_new_storage[8];
unsigned long global_array_new_storage[16];

void* operator new(unsigned long size) {
  return global_new_storage;
}

void operator delete(void* ptr) {
}

void* operator new[](unsigned long size) {
  return global_array_new_storage;
}

void operator delete[](void* ptr) {
}

struct CustomAllocated {
  int value;
  void* operator new(unsigned long size);
  void operator delete(void* ptr);
  void* operator new[](unsigned long size);
  void operator delete[](void* ptr);
};

unsigned long custom_new_storage[8];
unsigned long custom_array_new_storage[16];

void* CustomAllocated::operator new(unsigned long size) {
  return custom_new_storage;
}

void CustomAllocated::operator delete(void* ptr) {
}

void* CustomAllocated::operator new[](unsigned long size) {
  return custom_array_new_storage;
}

void CustomAllocated::operator delete[](void* ptr) {
}

Box::Box() {
}

Box::~Box() {
}

int main(void) {
  int* value = new int;
  int* initialized_value = new int(1);
  int* brace_value = new int{2};
  int* values = new int[3];
  Box* box = new Box;
  Box* boxes = new Box[2];
  CustomAllocated* custom = new CustomAllocated;
  CustomAllocated* customs = new CustomAllocated[2];
  delete value;
  delete initialized_value;
  delete brace_value;
  delete[] values;
  delete box;
  delete[] boxes;
  delete custom;
  delete[] customs;
  return 0;
}
