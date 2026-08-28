// Recursion that crosses the object boundary on every step.

int odd(int n);

int even(int n) { return n == 0 ? 1 : odd(n - 1); }

int main(void) { return even(10) * 2 + odd(7) * 3 + even(3); }
