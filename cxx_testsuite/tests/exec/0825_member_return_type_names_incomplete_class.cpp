// A member function may return a specialization naming its own, still
// incomplete, class (`absl::StatusOr<Socket> Accept();`) or a class that is
// only forward-declared (`absl::StatusOr<Publisher> CreatePublisher();`).
// That specialization must not be instantiated before the class is complete:
// StatusOr's move constructor depends on is_move_constructible<Socket>, which
// needs Socket's implicit special members.  A class that is never defined
// (pimpl) still gets its specialization when a call needs it.
#include <type_traits>
#include <utility>

template <class T, bool = std::is_move_constructible<T>::value>
struct MoveBase {
  MoveBase() = default;
  MoveBase(const MoveBase&) = default;
  MoveBase(MoveBase&&) = default;
};
template <class T>
struct MoveBase<T, false> {
  MoveBase() = default;
  MoveBase(const MoveBase&) = default;
  MoveBase(MoveBase&&) = delete;
};

template <class T>
struct StatusOr : MoveBase<T> {
  explicit StatusOr(T&& t) : value(std::move(t)) {}
  StatusOr(StatusOr&&) = default;
  T value;
};

template <class T>
struct Traits {
  using type = int;
};

template <class T>
struct Ptr {
  T* p = nullptr;
  bool empty() const { return p == nullptr; }
};

class Publisher;
struct Impl;
struct Client {
  StatusOr<Publisher> CreatePublisher(int fd) const;
  Ptr<Impl> Take() const { return Ptr<Impl>{}; }
  const Ptr<Impl>& Held() const { return held; }
  bool Probe() const { return Take().empty(); }
  Ptr<Impl> held;
};

class Publisher {
 public:
  explicit Publisher(int f) : fd(f) {}
  Publisher(const Publisher&) = delete;
  Publisher(Publisher&& other) : fd(other.fd) { other.fd = -1; }
  int fd;
};

StatusOr<Publisher> Client::CreatePublisher(int fd) const {
  return StatusOr<Publisher>(Publisher(fd));
}

struct Handle {
  Handle() = default;
  Handle(const Handle&) = delete;
  Handle(Handle&& other) : fd(other.fd) { other.fd = -1; }
  int fd = 3;
};

struct Socket : Handle {
  Socket() = default;
  explicit Socket(int f) { fd = f; }
  StatusOr<Socket> Accept() const;
  StatusOr<Socket> Accept(int f) const { return StatusOr<Socket>(Socket(f)); }
  static StatusOr<Socket> Create() { return StatusOr<Socket>(Socket(7)); }
  const StatusOr<Socket>* Find() const { return nullptr; }
  typename Traits<Socket>::type Kind() const { return 2; }
  int Use() const { return Accept(9).value.fd; }
};

StatusOr<Socket> Socket::Accept() const { return StatusOr<Socket>(Socket()); }

int main() {
  Socket s;
  StatusOr<Socket> a = s.Accept();
  StatusOr<Socket> b = std::move(a);
  if (b.value.fd != 3 || a.value.fd != -1) return 1;
  if (s.Accept(5).value.fd != 5) return 2;
  StatusOr<Socket> c = Socket::Create();
  if (c.value.fd != 7) return 3;
  if (s.Find() != nullptr) return 4;
  if (s.Kind() != 2) return 5;
  if (s.Use() != 9) return 6;
  static_assert(std::is_move_constructible<StatusOr<Socket>>::value, "");

  Client client;
  StatusOr<Publisher> p = client.CreatePublisher(4);
  StatusOr<Publisher> q = std::move(p);
  if (q.value.fd != 4 || p.value.fd != -1) return 7;
  if (!client.Take().empty() || !client.Held().empty() || !client.Probe())
    return 8;
  auto taken = client.Take();
  if (!taken.empty()) return 9;
  return 0;
}
