// std::allocate_shared takes its control block and object from the given
// allocator (rebound), constructs the object in place, destroys it when the
// last shared owner goes, and returns the block only after the last weak owner.
#include <stdio.h>
#include <memory>

static int allocations = 0;
static int deallocations = 0;
static int live_widgets = 0;

template <class T>
struct CountingAllocator {
  using value_type = T;
  int tag;
  explicit CountingAllocator(int t) : tag(t) {}
  template <class U>
  CountingAllocator(const CountingAllocator<U>& other) : tag(other.tag) {}
  T* allocate(size_t n) {
    ++allocations;
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  void deallocate(T* p, size_t) {
    ++deallocations;
    ::operator delete(p);
  }
};

template <class T, class U>
bool operator==(const CountingAllocator<T>& a, const CountingAllocator<U>& b) {
  return a.tag == b.tag;
}
template <class T, class U>
bool operator!=(const CountingAllocator<T>& a, const CountingAllocator<U>& b) {
  return a.tag != b.tag;
}

struct Widget : std::enable_shared_from_this<Widget> {
  int a;
  long b;
  Widget(int x, long y) : a(x), b(y) { ++live_widgets; }
  ~Widget() { --live_widgets; }
};

int main() {
  std::weak_ptr<Widget> weak;
  {
    std::shared_ptr<Widget> p =
        std::allocate_shared<Widget>(CountingAllocator<Widget>(7), 3, 40L);
    if (p->a != 3 || p->b != 40 || live_widgets != 1) return 1;
    if (allocations != 1 || deallocations != 0) return 2;
    if (p.use_count() != 1) return 3;
    std::shared_ptr<Widget> self = p->shared_from_this();
    if (self.get() != p.get() || p.use_count() != 2) return 4;
    weak = p;
  }
  if (live_widgets != 0) return 5;
  if (!weak.expired() || deallocations != 0) return 6;
  weak.reset();
  if (deallocations != 1) return 7;

  std::shared_ptr<int> i = std::allocate_shared<int>(CountingAllocator<char>(1), 42);
  if (*i != 42 || allocations != 2) return 8;
  i.reset();
  if (deallocations != 2) return 9;
  puts("pass");
  return 0;
}
