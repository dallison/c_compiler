// RUN: -std=c++17
// EXPECT_EXIT: 0

struct MessageLite {};
struct Message : MessageLite {};
struct GeneratedMessage : Message {};

int swap(MessageLite*, MessageLite*) { return 1; }
int swap(Message*, Message*) { return 2; }

int main() {
  GeneratedMessage first;
  GeneratedMessage second;
  return swap(&first, &second) == 2 ? 0 : 1;
}
