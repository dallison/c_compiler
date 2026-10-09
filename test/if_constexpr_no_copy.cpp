#include "google/protobuf/message_lite.h"
#include <type_traits>

int main() {
  if constexpr (!std::is_base_of<google::protobuf::Message, google::protobuf::MessageLite>::value) {
    return 1;
  }
  return 2;
}
