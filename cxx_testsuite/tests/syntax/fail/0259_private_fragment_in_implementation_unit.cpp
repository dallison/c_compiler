// RUN: -std=c++20
// EXPECT: Private module fragment is only permitted in a primary module interface unit
module syntax_impl_private;

int iface_fn();

module :private;

int private_helper() { return 0; }
