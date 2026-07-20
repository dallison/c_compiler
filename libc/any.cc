#include <any>

std::any::any(const std::any& other) : __storage{}, __ops(nullptr) {
  if (other.__ops != nullptr) {
    other.__ops->copy(&__storage, &other.__storage);
    __ops = other.__ops;
  }
}

std::any::any(std::any&& other) noexcept : __storage{}, __ops(nullptr) {
  if (other.__ops != nullptr) {
    other.__ops->move(&__storage, &other.__storage);
    __ops = other.__ops;
    other.__ops = nullptr;
  }
}

std::any::~any() {
  reset();
}

std::any& std::any::operator=(const std::any& other) {
  if (this == &other) {
    return *this;
  }
  any temporary(other);
  temporary.swap(*this);
  return *this;
}

std::any& std::any::operator=(std::any&& other) noexcept {
  if (this != &other) {
    reset();
    if (other.__ops != nullptr) {
      other.__ops->move(&__storage, &other.__storage);
      __ops = other.__ops;
      other.__ops = nullptr;
    }
  }
  return *this;
}

void std::any::reset() noexcept {
  if (__ops != nullptr) {
    const __operations* operations = __ops;
    __ops = nullptr;
    operations->destroy(&__storage);
  }
}

void std::any::swap(std::any& other) noexcept {
  if (this == &other) {
    return;
  }
  any temporary(static_cast<any&&>(other));
  if (__ops != nullptr) {
    __ops->move(&other.__storage, &__storage);
    other.__ops = __ops;
    __ops = nullptr;
  }
  if (temporary.__ops != nullptr) {
    temporary.__ops->move(&__storage, &temporary.__storage);
    __ops = temporary.__ops;
    temporary.__ops = nullptr;
  }
}
