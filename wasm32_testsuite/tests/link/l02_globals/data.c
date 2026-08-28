int counter = 10;
int table[4] = {1, 2, 3, 4};

// An initializer that names another object's address, which the link has to
// fill in rather than the compiler.
int* pointer_to_counter = &counter;

void bump(void) { counter++; }
