// RUN: -std=c++20
#include <memory>

int destructor_sum;

struct RuntimeMemoryBox {
  int value;

  RuntimeMemoryBox(int initial) : value(initial) {
  }

  ~RuntimeMemoryBox(void) {
    destructor_sum += value;
  }

  int read(void) {
    return value;
  }
};

struct RuntimeMemoryArrayBox {
  int value;

  RuntimeMemoryArrayBox(void) : value(0) {
  }

  ~RuntimeMemoryArrayBox(void) {
    destructor_sum += value;
  }
};

struct RuntimeMemoryBoxDeleter {
  int* calls;

  explicit RuntimeMemoryBoxDeleter(int* counter) : calls(counter) {
  }

  void operator()(RuntimeMemoryBox* ptr) {
    if (calls != 0) {
      *calls = *calls + ptr->value;
    }
    delete ptr;
  }
};

int main(void) {
  {
    std::unique_ptr<RuntimeMemoryBox> box(new RuntimeMemoryBox(3));
    if (box.get() == 0 || box->read() != 3) {
      return 1;
    }
    RuntimeMemoryBox* discarded = box.release();
    delete discarded;
  }
  if (destructor_sum != 3) {
    return 2;
  }

  std::unique_ptr<RuntimeMemoryBox> moved_from(new RuntimeMemoryBox(4));
  std::unique_ptr<RuntimeMemoryBox> moved_to(
      static_cast<std::unique_ptr<RuntimeMemoryBox>&&>(moved_from));
  if (moved_from.get() != 0 || moved_to->read() != 4) {
    return 3;
  }
  RuntimeMemoryBox* raw = moved_to.release();
  if (moved_to.get() != 0 || raw->read() != 4) {
    return 4;
  }
  delete raw;
  if (destructor_sum != 7) {
    return 5;
  }

  std::unique_ptr<RuntimeMemoryBox> assigned_move;
  std::unique_ptr<RuntimeMemoryBox> move_source(new RuntimeMemoryBox(6));
  assigned_move = static_cast<std::unique_ptr<RuntimeMemoryBox>&&>(move_source);
  if (assigned_move.get() == 0 || assigned_move->read() != 6) {
    return 6;
  }
  RuntimeMemoryBox* assigned_raw = assigned_move.release();
  delete assigned_raw;
  if (assigned_move.get() != 0 || destructor_sum != 13) {
    return 7;
  }

  int deleter_calls = 0;
  {
    RuntimeMemoryBoxDeleter deleter(&deleter_calls);
    std::unique_ptr<RuntimeMemoryBox, RuntimeMemoryBoxDeleter> custom(
        new RuntimeMemoryBox(13), deleter);
    custom.reset(new RuntimeMemoryBox(17));
  }
  if (deleter_calls != 30 || destructor_sum != 43) {
    return 8;
  }

  std::unique_ptr<RuntimeMemoryBox> made(new RuntimeMemoryBox(19));
  std::unique_ptr<RuntimeMemoryBox> swapped(new RuntimeMemoryBox(23));
  made.swap(swapped);
  if (made->read() != 23 || swapped->read() != 19) {
    return 9;
  }
  RuntimeMemoryBox* made_raw = made.release();
  RuntimeMemoryBox* swapped_raw = swapped.release();
  delete made_raw;
  delete swapped_raw;
  if (destructor_sum != 85) {
    return 10;
  }

  {
    std::unique_ptr<RuntimeMemoryArrayBox[]> boxes(
        new RuntimeMemoryArrayBox[2]);
    boxes[0].value = 7;
    boxes[1].value = 11;
    RuntimeMemoryArrayBox* discarded = boxes.release();
    delete[] discarded;
  }
  if (destructor_sum != 103) {
    return 11;
  }

  return 0;
}
