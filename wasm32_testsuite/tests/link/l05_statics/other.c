static int shared = 7;
static const char message[] = "two";

static int helper(int x) { return x * shared; }

int other_value(void) { return shared + message[0] - 't'; }

int other_helper(int x) { return helper(x); }
