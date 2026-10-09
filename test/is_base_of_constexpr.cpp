#include "google/protobuf/message_lite.h"
#include <type_traits>

static_assert(!std::is_base_of<google::protobuf::Message, google::protobuf::MessageLite>::value, "base");
int x = std::is_base_of<google::protobuf::Message, google::protobuf::MessageLite>::value ? 1 : 0;
