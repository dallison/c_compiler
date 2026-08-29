// Some x86-64 instructions leave their result in a register named by a separate
// `dest` node rather than one of their own.  The allocator released that
// register as soon as the destination node's use count ran out -- which happens
// at the very instruction producing the result -- so the next value was handed
// the register while later reads of the result were still to come, and they read
// the newcomer instead.
//
// __atomic_fetch_add is the shape that shows it: the fetched value lands in the
// register holding the addend, and it is read twice afterwards, once by the
// comparison lowered to a subtraction and once by the branch.

int main(void) {
  int value = 7;
  if (__atomic_fetch_add(&value, 3, __ATOMIC_SEQ_CST) != 7) {
    return 1;
  }
  if (value != 10) {
    return 2;
  }
  if (__atomic_fetch_sub(&value, 4, __ATOMIC_SEQ_CST) != 10) {
    return 3;
  }
  if (value != 6) {
    return 4;
  }
  // The fetched value read three times over, so freeing the register early
  // cannot be masked by the comparison happening to hold it.
  int again = 20;
  int fetched = __atomic_fetch_add(&again, 5, __ATOMIC_SEQ_CST);
  if (fetched != 20) {
    return 5;
  }
  if (fetched + again != 45) {
    return 6;
  }
  return 0;
}
