// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <memory>

int destroyed;

struct Value : std::enable_shared_from_this<Value> {
  int number;

  explicit Value(int value) : number(value) {
  }

  ~Value() {
    destroyed += number;
  }
};

struct Base {
  virtual ~Base() {
  }

  virtual int read() const {
    return 1;
  }
};

struct Derived : Base {
  int value;

  explicit Derived(int initial) : value(initial) {
  }

  int read() const override {
    return value;
  }
};

struct CountingDeleter {
  int* calls;

  void operator()(Value* pointer) {
    ++*calls;
    delete pointer;
  }
};

struct Holder {
  std::shared_ptr<int> value;

  explicit Holder(const std::shared_ptr<int>& source) : value(source) {
  }
};

int main() {
  std::weak_ptr<Value> observer;
  {
    std::shared_ptr<Value> first = std::make_shared<Value>(7);
    if (!first || first->number != 7 || first.use_count() != 1) {
      return 1;
    }

    std::shared_ptr<Value> second = first;
    observer = first;
    if (first.use_count() != 2 || observer.use_count() != 2 ||
        observer.expired()) {
      return 2;
    }

    std::shared_ptr<Value> locked = observer.lock();
    if (locked.get() != first.get() || locked.use_count() != 3) {
      return 3;
    }

    std::shared_ptr<Value> from_this = first->shared_from_this();
    std::weak_ptr<Value> weak_from_this = first->weak_from_this();
    if (from_this.get() != first.get() || first.use_count() != 4 ||
        weak_from_this.expired()) {
      return 4;
    }

    second.reset();
    locked.reset();
    from_this.reset();
    if (first.use_count() != 1 || observer.use_count() != 1) {
      return 5;
    }
  }

  if (!observer.expired() || observer.lock() || destroyed != 7) {
    return 6;
  }
  bool bad_weak_thrown = false;
  try {
    std::shared_ptr<Value> expired(observer);
    (void)expired;
  } catch (const std::bad_weak_ptr&) {
    bad_weak_thrown = true;
  }
  if (!bad_weak_thrown) {
    return 15;
  }

  int deleter_calls = 0;
  {
    CountingDeleter deleter{&deleter_calls};
    std::shared_ptr<Value> custom(new Value(11), deleter);
    CountingDeleter* stored = std::get_deleter<CountingDeleter>(custom);
    if (stored == 0 || stored->calls != &deleter_calls) {
      return 7;
    }
  }
  if (deleter_calls != 1 || destroyed != 18) {
    return 8;
  }

  {
    std::shared_ptr<Derived> derived = std::make_shared<Derived>(23);
    std::shared_ptr<Base> base = derived;
    std::shared_ptr<Derived> down = std::dynamic_pointer_cast<Derived>(base);
    std::shared_ptr<Base> up = std::static_pointer_cast<Base>(derived);
    if (!down || down->value != 23 || up->read() != 23 ||
        derived.use_count() != 4) {
      return 9;
    }

    std::shared_ptr<int> alias(derived, &derived->value);
    if (*alias != 23 || alias.use_count() != 5) {
      return 10;
    }
  }

  {
    std::shared_ptr<int[]> array(new int[3]);
    array[0] = 3;
    array[1] = 5;
    array[2] = 8;
    std::shared_ptr<int[]> copy = array;
    if (copy[0] + copy[1] + copy[2] != 16 || array.use_count() != 2) {
      return 11;
    }
  }

  {
    std::unique_ptr<Value> unique(new Value(13));
    std::shared_ptr<Value> shared(std::move(unique));
    if (unique.get() != 0 || !shared || shared->number != 13 ||
        shared->shared_from_this().get() != shared.get()) {
      return 12;
    }
    if (shared.use_count() != 1) {
      return 13;
    }
  }

  {
    std::shared_ptr<int> base = std::make_shared<int>(5);
    std::shared_ptr<int>* heap = new std::shared_ptr<int>(base);
    if (base.use_count() != 2 || **heap != 5) {
      return 16;
    }
    delete heap;
    if (base.use_count() != 1) {
      return 17;
    }
    Holder* holder = new Holder(base);
    if (base.use_count() != 2 || *holder->value != 5) {
      return 18;
    }
    delete holder;
    if (base.use_count() != 1) {
      return 19;
    }
  }

  if (destroyed != 31) {
    return 14;
  }

  return 0;
}
