// Everything this needs comes out of an archive, one member at a time.  The
// member holding unused_entry defines nothing anyone asks for and must not
// reach the output at all.

int square(int x);
int cube(int x);
extern int origin;

int main(void) { return square(3) + cube(2) + origin; }
