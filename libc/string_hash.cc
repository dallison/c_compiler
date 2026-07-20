#include <cstddef>

namespace std {
namespace __string_hash_detail {

size_t __fnv1a(const char* data, size_t size) noexcept {
  size_t hash = static_cast<size_t>(1469598103934665603ULL);
  for (size_t i = 0; i < size; i++) {
    hash ^= static_cast<size_t>(static_cast<unsigned char>(data[i]));
    hash *= static_cast<size_t>(1099511628211ULL);
  }
  return hash;
}

}  // namespace __string_hash_detail
}  // namespace std
