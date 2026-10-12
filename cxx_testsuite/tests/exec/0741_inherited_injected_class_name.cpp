// RUN: -std=c++17
// EXPECT_EXIT: 0

namespace messages {
struct MessageLite {
  int value;
};

struct Message : MessageLite {};
}  // namespace messages

namespace generated {
struct ConcreteMessage : messages::Message {
  static int read(MessageLite& message) { return message.value; }
};
}  // namespace generated

int main() {
  messages::MessageLite message{17};
  return generated::ConcreteMessage::read(message) == 17 ? 0 : 1;
}
