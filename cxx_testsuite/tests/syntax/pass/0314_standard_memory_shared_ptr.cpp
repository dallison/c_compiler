// RUN: -std=c++20
#include <memory>
#include <type_traits>

struct SharedBase {
  virtual ~SharedBase() {
  }
};

struct SharedDerived : SharedBase,
                       std::enable_shared_from_this<SharedDerived> {
  int value;

  explicit SharedDerived(int initial) : value(initial) {
  }
};

static_assert(std::is_same<
                  std::shared_ptr<SharedDerived>::element_type,
                  SharedDerived>::value);
static_assert(std::is_same<
                  std::shared_ptr<int[]>::element_type,
                  int>::value);
static_assert(std::is_copy_constructible<
                  std::shared_ptr<SharedDerived>>::value);
static_assert(std::is_move_constructible<
                  std::shared_ptr<SharedDerived>>::value);
static_assert(std::is_copy_constructible<
                  std::weak_ptr<SharedDerived>>::value);

void exercise_shared_ptr() {
  std::shared_ptr<SharedDerived> derived =
      std::make_shared<SharedDerived>(42);
  std::shared_ptr<SharedBase> base = derived;
  std::weak_ptr<SharedBase> weak = base;
  std::shared_ptr<SharedBase> locked = weak.lock();
  std::shared_ptr<SharedDerived> from_this =
      derived->shared_from_this();
  std::weak_ptr<SharedDerived> weak_from_this =
      derived->weak_from_this();

  std::shared_ptr<int> alias(derived, &derived->value);
  std::shared_ptr<SharedDerived> down =
      std::dynamic_pointer_cast<SharedDerived>(base);
  std::shared_ptr<const SharedBase> const_base =
      std::const_pointer_cast<const SharedBase>(base);

  std::shared_ptr<int[]> array(new int[2]);
  array[0] = *alias;

  (void)locked;
  (void)from_this;
  (void)weak_from_this;
  (void)down;
  (void)const_base;
}
