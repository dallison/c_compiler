#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"

namespace google {
namespace protobuf {
class MessageLite;
namespace internal {
class RepeatedPtrFieldBase;
class RustRepeatedMessageHelper {
 public:
  static void CopyFrom(const RepeatedPtrFieldBase& src,
                       RepeatedPtrFieldBase& dst);
};
}  // namespace internal
}  // namespace protobuf
}  // namespace google

// Pull in only the helper from repeated_ptr_field.h via include after forward decls won't work.
#include "google/protobuf/repeated_ptr_field.h"
