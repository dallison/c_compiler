//
//  syncstream.cc
//  c_compiler
//
//  Process-wide synchronization registry for <syncstream>.
//

#include <mutex>

namespace std {
namespace __syncstream_detail {

namespace {

struct mutex_entry {
  explicit mutex_entry(const void* buffer, mutex_entry* following)
      : key(buffer), next(following) {
  }

  const void* key;
  mutex lock;
  mutex_entry* next;
};

mutex& registry_lock() {
  static mutex lock;
  return lock;
}

mutex_entry*& registry_head() {
  static mutex_entry* head = nullptr;
  return head;
}

}  // namespace

mutex& __mutex_for(const void* wrapped) {
  lock_guard<mutex> guard(registry_lock());
  for (mutex_entry* entry = registry_head(); entry != nullptr;
       entry = entry->next) {
    if (entry->key == wrapped) {
      return entry->lock;
    }
  }
  mutex_entry* entry = new mutex_entry(wrapped, registry_head());
  registry_head() = entry;
  return entry->lock;
}

}  // namespace __syncstream_detail
}  // namespace std
