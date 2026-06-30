// Exercises the <new> header: direct calls to the global operator new /
// operator delete, the array, placement, nothrow and sized forms, plus the
// std::new_handler accessors (which rely on function-pointer comparison).
#include <new>

int counter_live = 0;

struct Counter {
  int id;
  Counter() {
    id = ++counter_live;
  }
  ~Counter() {
    --counter_live;
  }
};

static int handler_calls = 0;
static void on_no_memory() {
  ++handler_calls;
}

int main() {
  // Direct operator new / operator delete.
  void* raw = ::operator new(64);
  if (raw == 0) {
    return 1;
  }
  char* bytes = static_cast<char*>(raw);
  bytes[0] = 'a';
  bytes[63] = 'z';
  if (bytes[0] != 'a' || bytes[63] != 'z') {
    return 2;
  }
  ::operator delete(raw);

  // Sized deallocation forwards to the plain form.
  void* sized = ::operator new(48);
  ::operator delete(sized, static_cast<size_t>(48));

  // Array operator new[] / operator delete[].
  void* arr = ::operator new[](128);
  if (arr == 0) {
    return 3;
  }
  ::operator delete[](arr);

  // Non-throwing allocation.
  void* nothrow_raw = ::operator new(16, std::nothrow);
  if (nothrow_raw == 0) {
    return 4;
  }
  ::operator delete(nothrow_raw, std::nothrow);

  // new / delete expressions still run constructors and destructors.
  Counter* c = new Counter();
  if (counter_live != 1 || c->id != 1) {
    return 5;
  }
  delete c;
  if (counter_live != 0) {
    return 6;
  }

  // Placement new on caller-provided storage.
  unsigned char storage[sizeof(Counter)];
  Counter* placed = new (static_cast<void*>(storage)) Counter();
  if (placed != reinterpret_cast<Counter*>(storage) || counter_live != 1) {
    return 7;
  }
  placed->~Counter();
  if (counter_live != 0) {
    return 8;
  }

  // std::new_handler accessors (function-pointer round-trip and comparison).
  std::new_handler previous = std::set_new_handler(on_no_memory);
  if (std::get_new_handler() != on_no_memory) {
    return 9;
  }
  std::set_new_handler(previous);
  if (std::get_new_handler() != previous) {
    return 10;
  }

  // std::bad_alloc is usable as a type.
  std::bad_alloc oom;
  if (oom.what() == 0) {
    return 11;
  }

  return 0;
}
