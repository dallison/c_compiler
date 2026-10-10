#include "google/protobuf/repeated_ptr_field.h"

namespace google {
namespace protobuf {
namespace internal {

template <>
void RepeatedPtrFieldBase::MergeFrom<MessageLite>(
    const RepeatedPtrFieldBase& from, Arena* arena) {
  (void)from;
  (void)arena;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
