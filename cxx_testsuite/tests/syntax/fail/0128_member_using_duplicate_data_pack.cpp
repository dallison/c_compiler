// RUN: -std=c++20
// EXPECT: Duplicate class member value

struct DataUsingA {
  int value;
};

struct DataUsingB {
  int value;
};

template <class... Bases>
struct BadDataUsingPack : Bases... {
  using Bases::value...;
};

BadDataUsingPack<DataUsingA, DataUsingB> bad_data_using_pack;
