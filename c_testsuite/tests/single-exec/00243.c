#include <stdio.h>

typedef struct {
  unsigned value_set : 1;
  unsigned reserved : 7;
} Flags;

typedef struct {
  Flags flags;
  int value;
} Entry;

static int SetValue(Entry* entry, int value) {
  Entry* original = entry;
  entry->flags.value_set = 1;
  entry->value = value;
  return entry == original;
}

int main(void) {
  Entry entry = {{0, 0}, 0};
  if (!SetValue(&entry, 42)) return 1;
  if (entry.flags.value_set != 1 || entry.value != 42) return 2;
  puts("ok");
  return 0;
}
