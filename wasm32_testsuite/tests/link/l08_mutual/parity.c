int even(int n);

int odd(int n) { return n == 0 ? 0 : even(n - 1); }
