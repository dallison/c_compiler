// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <memory_resource>
#include <string>
#include <vector>

class counting_resource : public std::pmr::memory_resource {
 public:
  explicit counting_resource(std::pmr::memory_resource* upstream)
      : allocations(0), deallocations(0), bytes(0), upstream_(upstream) {
  }

  int allocations;
  int deallocations;
  unsigned long bytes;

 protected:
  void* do_allocate(unsigned long count, unsigned long alignment) override {
    ++allocations;
    bytes += count;
    return upstream_->allocate(count, alignment);
  }

  void do_deallocate(void* pointer, unsigned long count,
                     unsigned long alignment) override {
    ++deallocations;
    upstream_->deallocate(pointer, count, alignment);
  }

  bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

 private:
  std::pmr::memory_resource* upstream_;
};

struct alignas(64) over_aligned {
  int value;

  explicit over_aligned(int initial) : value(initial) {
  }
};

struct throwing_object {
  throwing_object() {
    throw 42;
  }
};

int main() {
  counting_resource resource(std::pmr::new_delete_resource());
  if (resource == *std::pmr::new_delete_resource() ||
      std::pmr::new_delete_resource() != std::pmr::new_delete_resource()) {
    return 1;
  }
  {
    std::pmr::polymorphic_allocator<int> allocator(&resource);
    int* values = allocator.allocate(3);
    allocator.construct(values, 4);
    allocator.construct(values + 1, 5);
    allocator.construct(values + 2, 6);
    if (values[0] + values[1] + values[2] != 15 ||
        allocator.resource() != &resource) {
      return 2;
    }
    allocator.destroy(values);
    allocator.destroy(values + 1);
    allocator.destroy(values + 2);
    allocator.deallocate(values, 3);

    over_aligned* object = allocator.new_object<over_aligned>(42);
    if (object->value != 42 ||
        reinterpret_cast<unsigned long>(object) % alignof(over_aligned) != 0) {
      return 3;
    }
    allocator.delete_object(object);

    int allocations_before_throw = resource.allocations;
    int deallocations_before_throw = resource.deallocations;
    try {
      allocator.new_object<throwing_object>();
      return 4;
    } catch (int value) {
      if (value != 42 ||
          resource.allocations != allocations_before_throw + 1 ||
          resource.deallocations != deallocations_before_throw + 1) {
        return 4;
      }
    }
  }
  {
    std::pmr::vector<int> values(&resource);
    values.push_back(3);
    values.push_back(5);
    values.push_back(8);
    if (values.size() != 3 || values[0] + values[1] + values[2] != 16) {
      return 5;
    }

    std::pmr::string text(
        "a string long enough to require polymorphic allocation", &resource);
    if (text.size() < 40 || text[0] != 'a') {
      return 6;
    }

  }

  std::pmr::memory_resource* old_default =
      std::pmr::set_default_resource(&resource);
  {
    std::pmr::polymorphic_allocator<long> allocator;
    if (allocator.resource() != &resource) {
      return 7;
    }
  }
  if (std::pmr::set_default_resource(nullptr) != &resource ||
      std::pmr::get_default_resource() != std::pmr::new_delete_resource()) {
    return 8;
  }
  std::pmr::set_default_resource(old_default);

  bool threw = false;
  try {
    std::pmr::null_memory_resource()->allocate(1);
  } catch (const std::bad_alloc&) {
    threw = true;
  }
  if (!threw) {
    return 9;
  }

  if (resource.allocations == 0 ||
      resource.allocations != resource.deallocations || resource.bytes == 0) {
    return 10;
  }
  return 0;
}
