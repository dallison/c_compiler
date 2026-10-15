// `constinit` may appear on a non-defining extern declaration
// ([dcl.constinit]); the definition carries the initializer.
constinit extern int counter;
constinit extern thread_local int* slot;
struct Box {
  int v;
};
extern constinit const Box kBox;
int UseBefore() { return counter + kBox.v + (slot == nullptr); }
constinit int counter = 40;
constinit thread_local int* slot = nullptr;
constinit const Box kBox = {1};
int main() { return UseBefore() == 42 ? 0 : 1; }
