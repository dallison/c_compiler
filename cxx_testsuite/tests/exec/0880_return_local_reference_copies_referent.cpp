// Returning a local reference by value copies the object it refers to; the
// reference is not a local object, so it is never built in the return slot
// (Subspace returned `shared_ptr<ActiveMessage>& m = active_messages_[id]`).
#include <memory>
#include <utility>
#include <vector>

static int copies, moves;

struct Box {
  int* p;
  explicit Box(int v) : p(new int(v)) {}
  Box(const Box& other) : p(new int(*other.p)) { copies++; }
  Box(Box&& other) : p(other.p) {
    other.p = nullptr;
    moves++;
  }
  ~Box() { delete p; }
};

struct Holder {
  Box items[2] = {Box(1), Box(2)};
};

static Box ThroughReference(Holder& holder, int i) {
  Box& box = holder.items[i];
  return box;
}

static Box ThroughConstReference(const Holder& holder) {
  const Box& box = holder.items[0];
  return box;
}

static Box ThroughRvalueReference(Holder& holder) {
  Box&& box = std::move(holder.items[1]);
  return box;
}

template <class T>
T ThroughDependentReference(std::vector<T>& items, int i) {
  T& item = items[i];
  return item;
}

static Box ThroughStructuredBinding(std::pair<Box, Box>& pair) {
  auto& [first, second] = pair;
  return second;
}

static std::shared_ptr<int> SharedThroughReference(
    std::vector<std::shared_ptr<int>>& all, int i) {
  std::shared_ptr<int>& element = all[i];
  *element += 1;
  return element;
}

int main() {
  Holder holder;
  Box a = ThroughReference(holder, 1);
  if (holder.items[1].p == nullptr || *holder.items[1].p != 2 || *a.p != 2)
    return 1;
  Box b = ThroughConstReference(holder);
  if (holder.items[0].p == nullptr || *b.p != 1) return 2;
  if (copies != 2 || moves != 0) return 3;

  std::vector<Box> boxes;
  boxes.emplace_back(5);
  copies = 0;
  Box c = ThroughDependentReference(boxes, 0);
  if (boxes[0].p == nullptr || *boxes[0].p != 5 || *c.p != 5 || copies != 1)
    return 4;

  std::pair<Box, Box> pair(Box(6), Box(7));
  Box d = ThroughStructuredBinding(pair);
  if (pair.second.p == nullptr || *d.p != 7) return 5;

  std::vector<std::shared_ptr<int>> all;
  for (int i = 0; i < 4; i++) all.push_back(std::make_shared<int>(i * 10));
  std::shared_ptr<int> shared = SharedThroughReference(all, 2);
  if (shared != all[2] || *shared != 21 || shared.use_count() != 2) return 6;
  shared.reset();
  if (all[2].use_count() != 1) return 7;

  Box e = ThroughRvalueReference(holder);
  if (*e.p != 2) return 8;
  return 0;
}
