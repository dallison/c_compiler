import partitioned;

int main() {
  if (partition_value() != 30) {
    return 1;
  }
  return primary_value() == 47 ? 0 : 2;
}
