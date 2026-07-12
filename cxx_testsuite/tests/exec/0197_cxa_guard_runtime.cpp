// RUN: -std=c++20

extern "C" int __cxa_guard_acquire(unsigned long long*);
extern "C" void __cxa_guard_release(unsigned long long*);
extern "C" void __cxa_guard_abort(unsigned long long*);

alignas(8) static unsigned long long first_guard;
alignas(8) static unsigned long long retry_guard;

int main() {
  if (__cxa_guard_acquire(&first_guard) != 1) {
    return 1;
  }
  __cxa_guard_release(&first_guard);
  if (__cxa_guard_acquire(&first_guard) != 0) {
    return 2;
  }

  if (__cxa_guard_acquire(&retry_guard) != 1) {
    return 3;
  }
  __cxa_guard_abort(&retry_guard);
  if (__cxa_guard_acquire(&retry_guard) != 1) {
    return 4;
  }
  __cxa_guard_release(&retry_guard);
  return __cxa_guard_acquire(&retry_guard) == 0 ? 0 : 5;
}
