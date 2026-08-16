// RUN: -std=c++26

template <typename T>
concept numeric = requires { ^^T; };

struct sample {
  int value;
  static int counter;
};

enum color { red, green };

int global_object = 1;

static_assert(^^int != ^^long);
static_assert(^^sample != ^^int);
static_assert(^^sample::value != ^^global_object);
static_assert(^^red != ^^green);
