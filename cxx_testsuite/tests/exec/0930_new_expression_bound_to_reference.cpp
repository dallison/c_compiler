// A new-expression is a prvalue pointer: a reference parameter or variable
// bound to it refers to a temporary holding the new pointer, not to the
// object it points at.  Statement expressions return non-class values by
// value too.
#include <stdio.h>

#include <utility>
#include <vector>

struct Payload {
  explicit Payload(int v) : value(v) {}
  int value;
};

static const void* stored;
void StoreVoid(const void* const& p) { stored = p; }
void StorePayload(Payload* const& p) { stored = p; }
bool IsRvalue(int*&&) { return true; }
bool IsRvalue(int*&) { return false; }

template <class T> struct Holder {
  void Put(T value) { items.push_back(new T(std::move(value))); }
  ~Holder() {
    for (std::vector<const void*>::const_iterator it = items.begin();
         it != items.end(); ++it) {
      delete static_cast<const T*>(*it);
    }
  }
  std::vector<const void*> items;
};

int main() {
  int failures = 0;

  StoreVoid(new int(5));
  int* five = (int*)stored;
  if (*five != 5) failures |= 1;
  delete five;

  StorePayload(new Payload(6));
  Payload* six = (Payload*)stored;
  if (six->value != 6) failures |= 2;
  delete six;

  const void* const& converted = new int(7);
  if (*(const int*)converted != 7) failures |= 4;
  delete (const int*)converted;

  int*&& bound = new int(8);
  if (*bound != 8) failures |= 8;
  delete bound;
  if (!IsRvalue(new int(0))) failures |= 16;

  {
    Holder<Payload> holder;
    holder.Put(Payload(9));
    if (static_cast<const Payload*>(holder.items[0])->value != 9) {
      failures |= 32;
    }
  }

  int x = 4;
  const int& copy = ({ x; });
  x = 10;
  if (copy != 4) failures |= 64;

  if (failures == 0) printf("ok\n");
  return failures;
}
