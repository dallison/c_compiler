// RUN: -std=c++23
// EXPECT: static lambda cannot have an explicit object parameter

auto function = [](this auto& self) static { return 1; };
