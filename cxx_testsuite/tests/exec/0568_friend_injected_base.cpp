// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class H>
struct HashStateBase {};

class MixingHashState : public HashStateBase<MixingHashState> {
  friend class MixingHashState::HashStateBase;
};

int main() { return 0; }
