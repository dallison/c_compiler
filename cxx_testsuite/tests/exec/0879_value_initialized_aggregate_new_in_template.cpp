// A new-expression in a template that value-initializes an aggregate with a
// non-trivial member constructs it in place: assigning a temporary into the
// fresh storage would run the member's assignment on garbage (Subspace's
// make_unique<OrdinalTracker>() freed a hash set table it never allocated).
#include <memory>
#include <new>
#include <string.h>
#include <utility>

static int assigns;

struct Owner {
  int* data;
  Owner() : data(new int(7)) {}
  Owner(Owner&& other) : data(other.data) { other.data = nullptr; }
  Owner& operator=(Owner&& other) {
    assigns++;
    delete data;
    data = other.data;
    other.data = nullptr;
    return *this;
  }
  ~Owner() { delete data; }
};

struct Tracker {
  Owner owner;
  unsigned long last = 0;
};

struct Unset {
  Owner owner;
  int zeroed;
  long also_zeroed;
};

struct Plain {
  int a;
  double b;
};

template <class T>
T* MakeEmpty(void* storage) {
  return new (storage) T();
}

template <class T, class... Args>
T* MakePack(void* storage, Args&&... args) {
  return new (storage) T(std::forward<Args>(args)...);
}

template <class T, class... Args>
T* MakeBraced(void* storage, Args&&... args) {
  return new (storage) T{std::forward<Args>(args)...};
}

template <class T>
T* InGarbage(T* (*make)(void*)) {
  void* storage = ::operator new(sizeof(T));
  memset(storage, 0xAB, sizeof(T));
  return make(storage);
}

template <class T>
void Destroy(T* object) {
  object->~T();
  ::operator delete(object);
}

int main() {
  Tracker* tracker = InGarbage<Tracker>(MakeEmpty<Tracker>);
  if (*tracker->owner.data != 7 || tracker->last != 0) return 1;
  Destroy(tracker);

  tracker = InGarbage<Tracker>(MakePack<Tracker>);
  if (*tracker->owner.data != 7 || tracker->last != 0) return 2;
  Destroy(tracker);

  tracker = InGarbage<Tracker>(MakeBraced<Tracker>);
  if (*tracker->owner.data != 7 || tracker->last != 0) return 3;
  Destroy(tracker);

  Unset* unset = InGarbage<Unset>(MakeEmpty<Unset>);
  if (*unset->owner.data != 7 || unset->zeroed != 0 ||
      unset->also_zeroed != 0) {
    return 4;
  }
  Destroy(unset);

  Plain* plain = InGarbage<Plain>(MakeEmpty<Plain>);
  if (plain->a != 0 || plain->b != 0.0) return 5;
  Destroy(plain);

  std::unique_ptr<Tracker> unique = std::make_unique<Tracker>();
  if (*unique->owner.data != 7 || unique->last != 0) return 6;

  if (assigns != 0) return 7;
  return 0;
}
