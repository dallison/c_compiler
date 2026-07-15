export module partitioned;

export import :detail;
import :impl;

export int primary_value() {
  return partition_value() + internal_partition_value() + 12;
}
