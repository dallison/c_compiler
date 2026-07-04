// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <utility>
#include <new>

struct Item {
  int value;

  explicit Item(int v) : value(v) {
  }

  Item(const Item& other) : value(other.value) {
  }

  Item(Item&& other) : value(other.value) {
    other.value = -1;
  }

  Item& operator=(Item&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
};

struct Forwarder {
  int seen;
  int marker[2];

  Forwarder() : seen(0) {
  }

  void inner(Item&& item) {
    Item local(std::move(item));
    seen = local.value;
  }

  void inner_placement(Item&& item) {
    Item* storage = static_cast<Item*>(::operator new(sizeof(Item)));
    new (storage) Item(std::move(item));
    seen = storage->value;
    storage->~Item();
    ::operator delete(storage);
  }

  void outer(Item&& item) {
    inner(std::move(item));
  }

  void outer_placement(Item&& item) {
    inner_placement(std::move(item));
  }

  int* begin() {
    return marker;
  }

  void insert_like(int* pos, Item&& item) {
    if (pos != marker + 1) {
      seen = -100;
      return;
    }
    inner_placement(std::move(item));
  }
};

int main(void) {
  Forwarder forwarder;
  Item item(7);
  forwarder.outer(std::move(item));
  if (forwarder.seen != 7) {
    return 1;
  }
  if (item.value != -1) {
    return 2;
  }
  Item placed(9);
  forwarder.outer_placement(std::move(placed));
  if (forwarder.seen != 9) {
    return 3;
  }
  if (placed.value != -1) {
    return 4;
  }
  Item inserted(11);
  forwarder.insert_like(forwarder.begin() + 1, std::move(inserted));
  if (forwarder.seen != 11) {
    return 5;
  }
  if (inserted.value != -1) {
    return 6;
  }
  return 0;
}
