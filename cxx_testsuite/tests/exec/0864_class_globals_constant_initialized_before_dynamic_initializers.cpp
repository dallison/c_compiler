// A static-storage class object whose initializer is a constant expression is
// constant-initialized ([basic.start.static]) whether or not it is const,
// constexpr, or constinit, and whether it is initialized explicitly or by a
// constexpr default constructor.  Protobuf relies on this: its `*_class_data_`
// tables (`const ClassDataFull` built by a `static constexpr auto` function)
// and `*_default_instance_` holders (a union around a polymorphic message) are
// read by other translation units' dynamic initializers, so they must already
// hold their values before any initializer runs.  Each object here is read by
// an initializer that runs before its definition.
struct Base {
  const void* p;
  unsigned off;
  bool lite;
  bool dyn = false;
  constexpr Base(const void* p, unsigned off, bool lite)
      : p(p), off(off), lite(lite) {}
};
struct Full : Base {
  constexpr Full(Base base, const int* d, void (*t)())
      : Base(base), r(), d(d), t(t) {}
  mutable const int* r;
  const int* d;
  void (*t)();
};
int kTable = 7;
struct Msg {
  static constexpr auto Gen();
};
constexpr auto Msg::Gen() { return Full{Base{&kTable, 4, false}, &kTable, nullptr}; }

struct P {
  constexpr P() : v(5) {}
  int v;
};
struct Q {
  constexpr Q(int x) : v(x) {}
  int v;
};

struct ConstantInitialized {};
struct ClassData {
  int id;
};
class MessageLite {
 public:
  constexpr MessageLite(const ClassData* d) : data_(d) {}
  virtual ~MessageLite() = default;
  virtual int Kind() const { return 1; }
  const ClassData* data_;
  void* arena_ = nullptr;
};
class Empty final : public MessageLite {
 public:
  constexpr Empty(ConstantInitialized);
  ~Empty() override;
  int Kind() const override { return 7; }
  static const ClassData class_data_;
  int cached_size_ = 0;
};
constexpr ClassData Empty::class_data_{42};
constexpr Empty::Empty(ConstantInitialized) : MessageLite(&class_data_) {}
Empty::~Empty() {}
struct EmptyDefaultTypeInternal {
  constexpr EmptyDefaultTypeInternal() : _instance(ConstantInitialized{}) {}
  ~EmptyDefaultTypeInternal() {}
  union {
    Empty _instance;
  };
};

extern const Full class_data;
extern const Q const_q;
extern P plain_p;
extern Q plain_q;
extern EmptyDefaultTypeInternal constinit_default_instance;
extern EmptyDefaultTypeInternal plain_default_instance;

static const Full* volatile class_data_seen = &class_data;
static int class_data_off = class_data_seen->off;
static const void* class_data_p = class_data_seen->p;
static const Q* volatile const_q_seen = &const_q;
static int const_q_v = const_q_seen->v;
static P* volatile plain_p_seen = &plain_p;
static int plain_p_v = plain_p_seen->v;
static Q* volatile plain_q_seen = &plain_q;
static int plain_q_v = plain_q_seen->v;
static const MessageLite* volatile constinit_seen =
    &constinit_default_instance._instance;
static int constinit_kind =
    constinit_seen->Kind() * 100 + constinit_seen->data_->id;
static const MessageLite* volatile plain_seen =
    &plain_default_instance._instance;
static int plain_kind = plain_seen->Kind() * 100 + plain_seen->data_->id;

const Full class_data = Msg::Gen();
const Q const_q = 3;
P plain_p;
Q plain_q{9};
constinit EmptyDefaultTypeInternal constinit_default_instance;
EmptyDefaultTypeInternal plain_default_instance;

int main() {
  if (class_data_off != 4 || class_data_p != &kTable) return 1;
  if (const_q_v != 3) return 2;
  if (plain_p_v != 5) return 3;
  if (plain_q_v != 9) return 4;
  if (constinit_kind != 742) return 5;
  if (plain_kind != 742) return 6;
  // The objects are still ordinary modifiable objects.
  plain_p.v = 11;
  plain_q.v = 12;
  if (plain_p_seen->v != 11 || plain_q_seen->v != 12) return 7;
  return 0;
}
