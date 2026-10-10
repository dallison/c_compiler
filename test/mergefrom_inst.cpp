#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"
#include <type_traits>

namespace google {
namespace protobuf {
class Message;
namespace internal {
class RepeatedPtrFieldBase {
 public:
  template <typename T>
  void MergeFromPattern(Arena* arena) {
    static_assert(std::is_base_of<MessageLite, T>::value, "");
    if constexpr (!std::is_base_of<Message, T>::value) {
      return;
    }
    void* (*copy)(Arena*, const void*) = Arena::CopyConstruct<T>;
    (void)copy;
    (void)arena;
  }
};
}  // namespace internal
}  // namespace protobuf
}  // namespace google

template void google::protobuf::internal::RepeatedPtrFieldBase::MergeFromPattern<
    google::protobuf::MessageLite>(google::protobuf::Arena*);
