// RUN: -std=c++20
// EXPECT: Private module fragment is only permitted in a primary module interface unit
export module syntax_part_private:details;

export int partition_iface();

module :private;

int helper() { return 0; }
