// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <atomic>
#include <rcu>
#if defined(__DAVECC_HAS_GUEST_THREADS__)
#include <thread>
#endif

#if defined(__cpp_lib_rcu)

#if __cpp_lib_rcu != 202306L
#error "unexpected __cpp_lib_rcu value"
#endif

struct node;

static int deleted;

struct counting_delete {
  void operator()(node* pointer) const noexcept;
};

struct node : std::rcu_obj_base<node, counting_delete> {
  int value;
  explicit node(int number) : value(number) {}
};

void counting_delete::operator()(node* pointer) const noexcept {
  ++deleted;
  delete pointer;
}

int main() {
  std::rcu_domain& first = std::rcu_default_domain();
  std::rcu_domain& second = std::rcu_default_domain();
  if (&first != &second) {
    return 1;
  }
  if (!first.try_lock()) {
    return 10;
  }
  if (!first.try_lock()) {
    first.unlock();
    return 11;
  }
  first.unlock();

  node* intrusive = new node(42);
  intrusive->retire(counting_delete());
  if (deleted != 0) {
    return 2;
  }
  first.unlock();
  std::rcu_barrier();
  if (deleted != 1) {
    return 3;
  }

  first.lock();
  node* external = new node(17);
  std::rcu_retire(external, counting_delete(), first);
  if (deleted != 1) {
    return 4;
  }
  first.unlock();
  std::rcu_barrier(first);
  if (deleted != 2) {
    return 5;
  }

#if defined(__DAVECC_HAS_GUEST_THREADS__)
  std::atomic<bool> locked(false);
  std::atomic<bool> release(false);
  std::atomic<bool> synchronized(false);
  std::thread reader([&] {
    first.lock();
    locked.store(true);
    while (!release.load()) {
      std::this_thread::yield();
    }
    first.unlock();
  });
  while (!locked.load()) {
    std::this_thread::yield();
  }
  std::thread writer([&] {
    std::rcu_synchronize(first);
    synchronized.store(true);
  });
  for (int i = 0; i != 100 && !synchronized.load(); ++i) {
    std::this_thread::yield();
  }
  if (synchronized.load()) {
    release.store(true);
    reader.join();
    writer.join();
    return 6;
  }
  release.store(true);
  reader.join();
  writer.join();
  if (!synchronized.load()) {
    return 7;
  }

  locked.store(false);
  release.store(false);
  std::atomic<bool> barrier_done(false);
  std::thread protected_reader([&] {
    first.lock();
    locked.store(true);
    while (!release.load()) {
      std::this_thread::yield();
    }
    first.unlock();
  });
  while (!locked.load()) {
    std::this_thread::yield();
  }
  std::rcu_retire(new node(23), counting_delete(), first);
  std::atomic<bool> barrier_started(false);
  std::thread barrier_thread([&] {
    barrier_started.store(true);
    std::rcu_barrier(first);
    barrier_done.store(true);
  });
  while (!barrier_started.load()) {
    std::this_thread::yield();
  }
  for (int i = 0; i != 100 && !barrier_done.load(); ++i) {
    std::this_thread::yield();
  }
  if (barrier_done.load() || deleted != 2) {
    release.store(true);
    protected_reader.join();
    barrier_thread.join();
    return 8;
  }
  release.store(true);
  protected_reader.join();
  barrier_thread.join();
  if (!barrier_done.load() || deleted != 3) {
    return 9;
  }
#else
  std::rcu_synchronize(first);
#endif

  return 0;
}

#else

int main() {
  return 0;
}

#endif
