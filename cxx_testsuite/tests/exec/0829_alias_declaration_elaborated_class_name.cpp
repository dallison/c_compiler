// `using A = struct S;` names the existing S in class and block scope; only a
// standalone `struct S;` declares a new nested or local class.
union Handler {
  void (*plain)(int);
  void (*info)(int, void*, void*);
};

struct Action {
  Handler handler;
  unsigned mask;
  int flags;
};

int Action(int, const struct Action*, struct Action*);

struct SignalData {
  const int signo;
  const char* const name;
  struct Action previous;
  using StructAction = struct Action;
};

static SignalData data[] = {
    {11, "SIGSEGV", SignalData::StructAction()},
    {4, "SIGILL", SignalData::StructAction()},
};

struct Outer {
  using Later = struct Fresh;
  Later* pointer;
};
struct Fresh {
  int value;
};

struct Nested {
  struct Action;
  struct Action {
    char bytes[3];
  };
};

int Local() {
  using A = struct Action;
  A a = {};
  a.flags = 5;
  struct Action* p = &a;
  return p->flags + (int)sizeof(A);
}

int LocalRedeclared() {
  struct Action;
  struct Action {
    char bytes[2];
  };
  return (int)sizeof(Action);
}

int main() {
  if (data[1].signo != 4 || data[0].previous.flags != 0 ||
      data[1].previous.handler.plain != nullptr)
    return 1;
  Fresh f = {7};
  Outer o = {&f};
  if (o.pointer->value != 7) return 2;
  if (sizeof(Nested::Action) != 3) return 3;
  if (Local() != 5 + (int)sizeof(struct Action)) return 4;
  if (LocalRedeclared() != 2) return 5;
  return 0;
}
