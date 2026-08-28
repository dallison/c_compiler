// The same names with internal linkage in both objects.  Nothing here may
// resolve against the other object's copy.

static int shared = 100;
static const char message[] = "one";

static int helper(int x) { return x + shared; }

int other_value(void);
int other_helper(int x);

int main(void) {
  shared += 1;
  return helper(2) + other_value() + other_helper(2) + message[0] - 'o';
}
