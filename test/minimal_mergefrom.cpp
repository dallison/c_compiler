#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"

namespace google {
namespace protobuf {
namespace internal {

template <typename T>
void MergeFromPattern(const void* from, Arena* arena) {
  static_assert(std::is_base_of<MessageLite, T>::value, "");
  if constexpr (!std::is_base_of<Message, T>::value) {
    return;
  }
  using CopyFn = void* (*)(Arena*, const void*);
  CopyFn copy = Arena::CopyConstruct<T>;
  (void)copy;
  (void)from;
  (void)arena;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
