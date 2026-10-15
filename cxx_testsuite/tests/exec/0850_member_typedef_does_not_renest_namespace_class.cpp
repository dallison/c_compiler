// `TimeUtil::Timestamp` is a member typedef for the namespace-scope class
// `pb::Timestamp`.  Constructing through it inside a member of TimeUtil must
// not make Timestamp a nested class of TimeUtil: its constructors keep their
// `pb::Timestamp` names, and the out-of-line one defined later still matches
// the delegating call (protobuf's util::TimeUtil::GetEpoch).
#include <cstring>
#include <typeinfo>

namespace pb {
struct Arena {};
class Timestamp {
 public:
  Timestamp() : Timestamp(nullptr) {}
  explicit Timestamp(Arena* arena);
  long seconds() const { return seconds_; }

 private:
  long seconds_;
};
}  // namespace pb

namespace util {
class TimeUtil {
 public:
  typedef pb::Timestamp Timestamp;
  static Timestamp GetEpoch();
};

TimeUtil::Timestamp TimeUtil::GetEpoch() { return Timestamp(); }
}  // namespace util

namespace pb {
Timestamp::Timestamp(Arena*) : seconds_(42) {}
}  // namespace pb

int main() {
  pb::Timestamp direct;
  if (util::TimeUtil::GetEpoch().seconds() != 42 || direct.seconds() != 42) {
    return 1;
  }
  return std::strcmp(typeid(pb::Timestamp).name(), "N2pb9TimestampE") == 0 ? 0
                                                                          : 2;
}
