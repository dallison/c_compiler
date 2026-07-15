import export_conflict;

int local_conflict_fn() { return 2; }

int main() {
  return conflict_fn() == 1 ? 0 : 1;
}
