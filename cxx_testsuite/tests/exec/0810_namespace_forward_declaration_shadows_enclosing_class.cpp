// RUN: -std=c++17
// EXPECT_EXIT: 0

// A standalone `class X;` in a nested namespace declares a new X there even
// when an enclosing (or global) scope already has an X ([dcl.type.elab]).
// Binding the forward declaration to the outer X while completing a distinct
// inner X makes the two X* types print the same name but refuse conversion.

class Scheduler {
 public:
  int outer = 1;
};

namespace subspace::co20 {
class Scheduler;
class Event {
 public:
  explicit Event(Scheduler* sched = nullptr) : sched_(sched) {}
  void set_scheduler(Scheduler* sched) { sched_ = sched; }
  Scheduler* get() const { return sched_; }

 private:
  Scheduler* sched_ = nullptr;
  friend class Scheduler;
};

class Scheduler {
 public:
  Scheduler() : wake_event_(this) {}
  Event& wake_event() { return wake_event_; }
  int inner = 2;

 private:
  Event wake_event_;
};
}  // namespace subspace::co20

namespace subspace {
struct Server {
  Server() { req_event_.set_scheduler(&sched_); }
  co20::Scheduler sched_;
  co20::Event req_event_;
};
}  // namespace subspace

int main() {
  subspace::Server server;
  if (server.sched_.inner != 2) {
    return 1;
  }
  if (server.req_event_.get() != &server.sched_) {
    return 2;
  }
  if (server.sched_.wake_event().get() != &server.sched_) {
    return 3;
  }
  Scheduler outer;
  if (outer.outer != 1) {
    return 4;
  }
  return 0;
}
