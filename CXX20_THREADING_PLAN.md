# C++20 Hosted Threading Plan

Supported execution targets: x86-64, AArch64, ARM, and RISC-V.
P-code and 65(C)02 remain intentionally threadless and receive compile-time
diagnostics for the standard threading headers.

Compiler or runtime failures discovered during this work must be fixed at
their source. Do not add library or test workarounds.

## Step 1: Runtime foundation

- [x] Define hosted syscall APIs for thread detach, sleeping, CPU count, and
      address-based wait/wake.
- [x] Add C runtime wrappers and condition-variable primitives in
      `libc/include/threads.h` and `libc/threads.c`.
- [x] Implement per-address blocking wait queues for x86-64.
- [x] Implement the same wait-queue semantics for AArch64.
- [x] Implement the same wait-queue semantics for ARM.
- [x] Implement the same wait-queue semantics for RISC-V.
- [x] Recheck waited values while holding the queue lock to prevent lost
      wakeups.
- [x] Add safe joined- and detached-thread resource reclamation, preserving
      exactly-once TLS finalization and shutdown behavior.
- [x] Replace polling atomic waits and no-op notifications with the runtime
      wait/wake primitives.
- [x] Add C runtime tests for detach/reclamation, sleep, condition variables,
      wait/wake races, timeouts, and TLS cleanup.
- [x] Run runtime and atomic regressions on all four hosted targets.
- [x] Review the complete Step 1 diff and test results.

**Review gate:** Stop after Step 1. Do not begin Step 2 until the user reviews
and approves the runtime foundation.

## Step 2: `std::thread`

- [ ] Add `std::thread`, `thread::id`, join/detach, native handles, and
      hardware concurrency.
- [ ] Add `this_thread::get_id`, `yield`, `sleep_for`, and `sleep_until`.
- [ ] Support move-only callables and arguments with exactly-once cleanup.
- [ ] Add the minimal `system_error` support required for thread failures.
- [ ] Add syntax, execution, and unsupported-profile tests.
- [ ] Review Step 2 before proceeding.

## Step 3: Blocking synchronization

- [ ] Add `condition_variable` and `condition_variable_any`, including
      predicate and timed waits.
- [ ] Add `counting_semaphore` and `binary_semaphore`.
- [ ] Add `latch`.
- [ ] Add reusable `barrier` and completion callbacks.
- [ ] Add hosted cross-backend tests.
- [ ] Review Step 3 before proceeding.

## Step 4: Stop tokens and `jthread`

- [ ] Add `stop_token`, `stop_source`, `stop_callback`, and `nostopstate`.
- [ ] Make callback registration, execution, and destruction race-safe.
- [ ] Add `jthread` stop-token injection, request-stop, move, and destructor
      semantics.
- [ ] Add hosted cross-backend tests.
- [ ] Review Step 4 before proceeding.

## Step 5: Integration and completion

- [ ] Export completed headers through `libc/modules/std.hpp`.
- [ ] Add p-code and 65(C)02 diagnostics and update profile documentation.
- [ ] Run syntax, libc runtime, dedicated threading, and full hosted execution
      suites.
- [ ] Check off the threading item in `CXX20_CXX23_REMAINING.md`.
- [ ] Perform final review before commit.
