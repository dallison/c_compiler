// Vtables of classes in different namespaces with the same name are distinct,
// and a vtable first named by a constant-initialized object is still emitted.
namespace a {
struct F {
  virtual int f() const { return 1; }
};
}  // namespace a

namespace b {
struct F {
  virtual int f() const { return 2; }
};
}  // namespace b

namespace proto {
struct Message {
  constexpr Message() {}
  virtual ~Message() {}
  virtual int Value() const = 0;
};

struct FeatureSet final : Message {
  constexpr FeatureSet() : value(7) {}
  explicit FeatureSet(int v);
  ~FeatureSet() override;
  int Value() const override { return value; }
  int value;
};

struct DefaultInstance {
  constexpr DefaultInstance() : instance() {}
  ~DefaultInstance() {}
  union {
    FeatureSet instance;
  };
};

DefaultInstance default_feature_set;

FeatureSet::FeatureSet(int v) : value(v) {}
FeatureSet::~FeatureSet() {}
}  // namespace proto

int CallF(const a::F& x) { return x.f(); }
int CallF(const b::F& x) { return x.f(); }
int CallValue(const proto::Message& m) { return m.Value(); }

int main() {
  a::F fa;
  b::F fb;
  if (CallF(fa) != 1 || CallF(fb) != 2) {
    return 1;
  }
  if (CallValue(proto::default_feature_set.instance) != 7) {
    return 2;
  }
  proto::FeatureSet* set = new proto::FeatureSet(9);
  int value = CallValue(*set);
  delete set;
  return value == 9 ? 0 : 3;
}
