// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(__cpp_lib_allocate_at_least >= 202302L, "");
static_assert(__cpp_lib_out_ptr >= 202106L, "");
static_assert(__cpp_lib_atomic_shared_ptr >= 201711L, "");
static_assert(__cpp_lib_smart_ptr_for_overwrite >= 202002L, "");
static_assert(__cpp_lib_assume_aligned >= 201811L, "");
static_assert(std::allocator_traits<std::allocator<int>>::
                  propagate_on_container_move_assignment::value,
              "");
static_assert(std::is_same<std::allocator_traits<std::allocator<int>>::rebind_alloc<long>,
                           std::allocator<long>>::value,
              "");
static_assert(!std::atomic<std::shared_ptr<int>>::is_always_lock_free, "");

template <class T, class = void>
struct HasConstruct : std::false_type {};

template <class T>
struct HasConstruct<
    T, std::void_t<decltype(std::declval<std::allocator<T>&>().construct(
           std::declval<T*>(), std::declval<T>()))>> : std::true_type {};

template <class T, class = void>
struct HasRebind : std::false_type {};

template <class T>
struct HasRebind<T, std::void_t<typename std::allocator<T>::template rebind<T>>>
    : std::true_type {};

static_assert(!HasConstruct<int>::value, "");
static_assert(!HasRebind<int>::value, "");

struct Node {
  static int live;
  int value;
  explicit Node(int v) : value(v) { ++live; }
  ~Node() { --live; }
};

int Node::live = 0;

void publish(Node** slot) { *slot = new Node(7); }

void replace(Node** slot) {
  delete *slot;
  *slot = new Node(8);
}

template <class T>
struct Extra {
  using value_type = T;
  T* allocate(size_t n) {
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  std::allocation_result<T*> allocate_at_least(size_t n) {
    return {allocate(n + 2), n + 2};
  }
  void deallocate(T* pointer, size_t) { ::operator delete(pointer); }
};

int main() {
  std::allocator<int> alloc;
  int* raw = std::allocator_traits<std::allocator<int>>::allocate(alloc, 1);
  std::allocator_traits<std::allocator<int>>::construct(alloc, raw, 5);
  if (*raw != 5) {
    return 1;
  }
  std::allocator_traits<std::allocator<int>>::destroy(alloc, raw);
  alloc.deallocate(raw, 1);

  std::allocation_result<int*> least = alloc.allocate_at_least(3);
  if (least.ptr == nullptr || least.count < 3) {
    return 2;
  }
  least.ptr[2] = 9;
  if (least.ptr[2] != 9) {
    return 3;
  }
  alloc.deallocate(least.ptr, least.count);

  Extra<int> extra;
  std::allocation_result<int*> grown =
      std::allocator_traits<Extra<int>>::allocate_at_least(extra, 4);
  if (grown.count != 6 || grown.ptr == nullptr) {
    return 4;
  }
  extra.deallocate(grown.ptr, grown.count);

  std::unique_ptr<Node> held(new Node(1));
  if (Node::live != 1) {
    return 5;
  }
  publish(std::out_ptr(held));
  if (Node::live != 1 || !held || held->value != 7) {
    return 6;
  }

  replace(std::inout_ptr(held));
  if (Node::live != 1 || !held || held->value != 8) {
    return 7;
  }
  held.reset();
  if (Node::live != 0) {
    return 8;
  }

  std::atomic<std::shared_ptr<int>> shared(std::make_shared<int>(3));
  std::shared_ptr<int> loaded = shared.load();
  if (!loaded || *loaded != 3) {
    return 9;
  }
  std::shared_ptr<int> previous = shared.exchange(std::make_shared<int>(4));
  if (!previous || *previous != 3) {
    return 10;
  }
  loaded = shared.load();
  if (!loaded || *loaded != 4) {
    return 11;
  }
  std::shared_ptr<int> expected = loaded;
  if (!shared.compare_exchange_strong(expected, std::make_shared<int>(5))) {
    return 12;
  }
  std::shared_ptr<int> wrong = std::make_shared<int>(0);
  if (shared.compare_exchange_strong(wrong, std::make_shared<int>(6))) {
    return 13;
  }
  if (!wrong || *wrong != 5) {
    return 14;
  }
  shared.wait(std::make_shared<int>(0));

  std::atomic<std::weak_ptr<int>> weak(std::weak_ptr<int>(shared.load()));
  std::shared_ptr<int> locked = weak.load().lock();
  if (!locked || *locked != 5) {
    return 15;
  }

  alignas(int) unsigned char bytes[sizeof(int) * 4];
  int* slot = reinterpret_cast<int*>(bytes);
  std::ranges::uninitialized_value_construct(
      std::ranges::subrange(slot, slot + 4));
  if (slot[0] != 0 || slot[3] != 0) {
    return 16;
  }
  std::destroy(slot, slot + 4);
  std::ranges::uninitialized_fill(std::ranges::subrange(slot, slot + 4), 6);
  if (slot[1] != 6 || slot[3] != 6) {
    return 17;
  }
  std::destroy(slot, slot + 4);

  std::vector<int> source;
  source.push_back(1);
  source.push_back(2);
  std::vector<int> dest;
  dest.push_back(9);
  dest = static_cast<std::vector<int>&&>(source);
  if (dest.size() != 2 || dest[0] != 1 || dest[1] != 2 || !source.empty()) {
    return 18;
  }
  return 0;
}
