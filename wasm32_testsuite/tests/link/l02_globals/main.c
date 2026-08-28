// Data defined in one object and read and written from another.

extern int counter;
extern int table[4];
extern int* pointer_to_counter;

void bump(void);

int main(void) {
  bump();
  bump();
  counter += table[2];
  *pointer_to_counter += 1;
  return counter + table[0] + table[3];
}
