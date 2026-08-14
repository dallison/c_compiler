// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <atomic>
#include <memory>
#include <thread>

struct ThreadValue {
  std::atomic<int>* destructions;
  int value;

  ThreadValue(std::atomic<int>* count, int initial)
      : destructions(count), value(initial) {
  }

  ~ThreadValue() {
    destructions->fetch_add(1);
  }
};

constexpr auto thread_value_data_size =
    sizeof(std::atomic<int>*) + sizeof(int);
constexpr auto thread_value_expected_size =
    ((thread_value_data_size + alignof(ThreadValue) - 1) /
     alignof(ThreadValue)) *
    alignof(ThreadValue);
static_assert(sizeof(ThreadValue) == thread_value_expected_size);

int main() {
  std::atomic<int> destructions{0};
  std::atomic<int> sum{0};
  std::shared_ptr<ThreadValue> root =
      std::make_shared<ThreadValue>(&destructions, 3);

  {
    auto work = [root, &sum] {
      for (int i = 0; i < 200; ++i) {
        std::shared_ptr<ThreadValue> copy = root;
        sum.fetch_add(copy->value);
      }
    };
    if (root.use_count() != 2) {
      return 10 + root.use_count();
    }

    std::thread first(work);
    std::thread second(work);
    root.reset();
    first.join();
    second.join();

    if (sum.load() != 1200) {
      return 1;
    }
    if (destructions.load() != 0) {
      return 2;
    }
  }

  if (destructions.load() != 1) {
    return 3;
  }
  return 0;
}
