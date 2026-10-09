#include "google/protobuf/message_lite.h"
#include <type_traits>

int main() {
  if constexpr (std::is_base_of<google::protobuf::Message, google::protobuf::MessageLite>::value) {
    void* (*fn)(google::protobuf::Arena*, const void*) =
        google::protobuf::Arena::CopyConstruct<google::protobuf::MessageLite>;
    (void)fn;
    return 2;
  }
  return 1;
}
