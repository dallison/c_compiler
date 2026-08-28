// Needs a member that comes later in the archive, so pulling this one in has
// to make the linker go round again.
int square(int x);

int cube(int x) { return square(x) * x; }
