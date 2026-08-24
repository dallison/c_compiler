#include <setjmp.h>
#include <stdio.h>

static jmp_buf primary_env;
static jmp_buf secondary_env;
static int secondary_completed;

static void jump_with_value(int value) {
  longjmp(primary_env, value);
}

static void jump_through_frames(int depth) {
  volatile int stack_marker = depth + 1;
  if (depth != 0) {
    jump_through_frames(depth - 1);
  }
  if (stack_marker == 1) {
    longjmp(primary_env, 77);
  }
}

static void use_secondary_environment(void) {
  int value = setjmp(secondary_env);
  if (value == 0) {
    longjmp(secondary_env, 19);
  }
  if (value != 19) {
    longjmp(primary_env, 99);
  }
  secondary_completed = 1;
  longjmp(primary_env, -7);
}

static int stack_still_usable(int value) {
  volatile int local = value + 5;
  return local + 2;
}

int main(void) {
  int value = setjmp(primary_env);
  if (value == 0) {
    jump_with_value(42);
  }
  if (value != 42) return 1;

  value = setjmp(primary_env);
  if (value == 0) {
    jump_with_value(0);
  }
  if (value != 1) return 2;

  value = setjmp(primary_env);
  if (value == 0) {
    jump_through_frames(4);
  }
  if (value != 77) return 3;
  if (stack_still_usable(5) != 12) return 4;

  value = setjmp(primary_env);
  if (value == 0) {
    use_secondary_environment();
  }
  if (value != -7) return 5;
  if (!secondary_completed) return 6;

  puts("ok");
  return 0;
}
