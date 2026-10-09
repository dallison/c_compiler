#include "google/protobuf/message_lite.h"
#include <type_traits>

template <typename T>
int pick() {
  if constexpr (!std::is_base_of<google::protobuf::Message, T>::value) {
    return 1;
  }
  void* (*fn)(google::protobuf::Arena*, const void*) =
      google::protobuf::Arena::CopyConstruct<T>;
  (void)fn;
  return 2;
}

int test_ml() { return pick<google::protobuf::MessageLite>(); }
