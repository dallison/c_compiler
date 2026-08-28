int callback(int a);

int add(int a, int b) { return a + b; }

// Calls back into the object that called this one, so the link has to
// resolve references running each way.
int scale(int a) { return callback(a) + 1; }
