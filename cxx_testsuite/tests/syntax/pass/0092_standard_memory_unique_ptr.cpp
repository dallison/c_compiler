// RUN: -std=c++20
#include <memory>

struct MemoryBox {
  int value;

  MemoryBox(int initial) : value(initial) {
  }

  int read(void) {
    return value;
  }
};

struct MemoryBoxDeleter {
  int* calls;

  MemoryBoxDeleter(void) : calls(0) {
  }

  explicit MemoryBoxDeleter(int* counter) : calls(counter) {
  }

  void operator()(MemoryBox* ptr) {
    if (calls != 0) {
      *calls = *calls + ptr->value;
    }
    delete ptr;
  }
};

int main(void) {
  std::unique_ptr<MemoryBox> empty;
  if (empty.get() != 0) {
    return 1;
  }

  std::unique_ptr<MemoryBox> box(new MemoryBox(3));
  if (box.get() == 0 || (*box).value != 3 || box->read() != 3) {
    return 2;
  }

  MemoryBox* raw = box.release();
  if (box.get() != 0 || raw->read() != 3) {
    return 3;
  }
  delete raw;

  box.reset(new MemoryBox(4));
  std::unique_ptr<MemoryBox> moved(std::move(box));
  std::unique_ptr<MemoryBox> assigned;
  assigned = std::move(moved);
  if (assigned.get() == 0 || box.get() != 0 || moved.get() != 0) {
    return 4;
  }
  if (assigned == nullptr || nullptr == assigned || assigned != nullptr) {
    return 5;
  }
  assigned = nullptr;
  if (assigned != nullptr) {
    return 6;
  }

  std::unique_ptr<MemoryBox> made = std::make_unique<MemoryBox>(8);
  if (made.get() == 0 || made->read() != 8) {
    return 7;
  }

  std::unique_ptr<MemoryBox> swapped(new MemoryBox(9));
  made.swap(swapped);
  if (made->read() != 9 || swapped->read() != 8) {
    return 8;
  }

  int deleter_calls = 0;
  {
    MemoryBoxDeleter deleter(&deleter_calls);
    std::unique_ptr<MemoryBox, MemoryBoxDeleter> custom(
        new MemoryBox(10), deleter);
    custom.reset(new MemoryBox(11));
  }
  if (deleter_calls != 21) {
    return 9;
  }

  std::unique_ptr<int[]> values(new int[3]);
  values[0] = 1;
  values[1] = 2;
  values[2] = 3;
  if (values[0] + values[1] + values[2] != 6) {
    return 10;
  }

  std::unique_ptr<int[]> moved_values(std::move(values));
  std::unique_ptr<int[]> assigned_values;
  assigned_values = std::move(moved_values);
  if (values.get() != 0 || moved_values.get() != 0 ||
      assigned_values[0] != 1) {
    return 11;
  }

  return 0;
}
