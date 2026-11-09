# Register-copy coalescing port — handoff

Status as of the commit that adds this file. The work ports the AArch64
copy-coalescing optimization (originally `cd84f334`) to every other
backend that shares the same register-allocator template, plus a
correctness hardening of the original AArch64 implementation. All of it
is committed together; validation is complete for some targets and
partial for others (see per-target status). Read the "Remaining work"
section before touching the unfinished parts.

## What the optimization does

In `AllocateRegisterOnce`/`AllocateUsingDest`, a pure copy
`mv src -> varreg` (opcodes `mv`/`fmv_s`/`fmv_d`, RISC-V-family and
xtensa; `mv`/`mov`/`fmov`/`fmv_s`/`fmv_d` on ARM/AArch64; `mv`/`fmv_s`/
`fmv_d` on x86) whose source dies at the copy is eliminated one of two
ways, implemented per backend in
`TryCoalesceCopyIntoVariableRegister`:

- **Give**: if the variable register (`varreg`) has no register yet, it
  inherits the source's register (`varreg->reg = src->reg`,
  `reg->owner = varreg`, saved-register bookkeeping via
  `NoteUsedSavedRegister` so the prologue saves it).
- **Retarget**: if the varreg already has a register, the source's
  definition (same block, dies at the copy) is rewritten to write the
  varreg's register directly
  (`TryRetargetCopySourceDefinition`).

Emitters already suppress same-register moves on all these targets, so
no emitter changes were needed outside the original AArch64 work.

Guards shared by all ports (helpers `VariableRegisterDefinition`,
`VariableRegisterDeadAfterCopy`, `TryRetargetCopySourceDefinition`,
`RegisterHasOtherLiveHolder` in each `*_reg_alloc.c`):

- Source not fixed/spilled, `src->dest == NULL`, `src->uses == 1`, not
  live out of the copy block (`output_ids`); varreg sources require the
  single-definition dead-after-copy proof (no short-lived exception
  outside AArch64).
- Retarget rejects fixed-result definitions (calls, `resulti/resultf/
  resultd` (+`resulth`/`resultv` on x86), `idiv`/`div`/`mod` on x86,
  atomics, `tp` where applicable), spilled varregs, stranger-owned
  target registers, intervening reads/writes of the target register,
  and intervening calls when the target is caller-saved.
- **Live-holder scan** (`RegisterHasOtherLiveHolder`): reject when any
  other instruction anywhere in the function holds the same register
  with an unprocessed user or is live out of its block. This is the fix
  for the ARM codecvt bug below; register ownership in these allocators
  is per-block and approximate across regions, so a register free in
  this block may hold a value live on a sibling path.
- Give additionally rejects: another varreg's home register, a value
  live into the copy block, the hidden struct-return varreg's register,
  and (RISC-V/RV32/Xtensa) the return-address register.
- In calling functions the give path only allows callee-saved registers
  (RISC-V-family form: `num_calls > 0 || !CanUseTemp(...)` implies
  saved-only; AArch64/ARM use the pool/`CanUseTemp` form).

Target-specific notes:

- **AArch64** (`aarch64_reg_alloc.c`): only the hardening was added
  this round (live-holder scan + varreg-home/live-in guards in give and
  retarget). The base optimization was already in `cd84f334`.
- **RISC-V** (`risc_v_reg_alloc.c`): hook is in `AllocateUsingDest`;
  copies reload/ensure operands *before* dest allocation
  (`operands_ready`). Return-address register `ra` is in the temp pool
  and must never be inherited (regex `0515`/`0516` crashed without this
  guard). `NoteUsedSavedRegister` covers int/float/vector.
- **risc_v32** (`risc_v32_reg_alloc.c`): same as RISC-V minus vector
  registers; `RV32_RET_REG` guard included.
- **ARM** (`arm_reg_alloc.c`): hook inside the dest path of
  `AllocateRegisterOnce`. Rejects 128-bit (NEON quad) src/varreg.
  `InstructionHoldsRegister` also treats the `reg+2` half of a 128-bit
  float pair as held, in the holder/inputs/intervening scans.
- **x86** (`x86_reg_alloc.c`, covers x86-64 and i386 profiles): hook in
  `AllocateUsingDest` (skips only the allocation step; the rest of the
  function, including `SyncSpilledVarReg`, still runs). Logical slots
  alias physical registers, so all identity checks go through
  `SamePhysicalRegister`; give finishes with the backend's own
  `AssignRegister`. Retarget denylist also includes `idiv`/`div`/`mod`
  (implicit rax/rdx results), `resulth`, `resultv`, `structreturn`.
  Give rejects `X86PhysicalReserved` registers (r11, xmm14/xmm15
  scratch). Two-address SSE definitions are safe to retarget because
  the emitter materializes operand 0 into the result register
  (`PrintMovToDestIfNeeded`).
- **Xtensa** (`xtensa_reg_alloc.c`): same as RV32; `XTENSA_RET_REG`
  is a0 (=0), also in the temp pool, guard included.

Also fixed in passing: `target_owner` in the retarget path was declared
`TargetRegister*` but `TargetRegister::owner` is a `TargetInstruction*`
in all six backends (compiled only as a warning). Now declared
correctly everywhere.

Not ported, deliberately:

- **wasm32**: `wasm32_reg_alloc.c` is a 56-line stub (stack machine).
- **6502/65c02, p_code, bpf**: accumulator/stack models without this
  varreg allocator template.

## Validation status

Harness notes: direct-script suite runs use
`cxx_testsuite/run_exec_tests.sh` (see commands at the bottom). In this
local direct-script environment the RISC-V/ARM expected-fail files are
effectively empty, so results below are stated as *failure set vs.
baseline* (same harness, compiler without the port), which is the
meaningful comparison. The C++ exec corpus is
`cxx_testsuite/tests/exec` (1032 tests).

- **AArch64**: BBC emulator bit-identical to clang (`ram same screen
  same`), ~26.4G retired instructions (coalescing win preserved after
  the hardening scan). Spot tests pass (`0492`, `0515`, `0516`, `1052`,
  `0470`, `coalstress.c`). NOT rerun after the hardening scan: full O2
  and O0 exec suites (they passed at `cd84f334` without the scan; the
  scan only rejects more coalesces).
- **RISC-V**: full direct O2 corpus — failure set identical to baseline
  (12 environmental failures: `0111 0114 0115 0118 0119 0120 0124 0128
  0275 0403 0474 0475`). Regex `0515`/`0516` pass (they were the `ra`
  regression). Done.
- **ARM**: full direct O2 corpus — failure set identical to baseline
  after the live-holder fix (`0492_standard_codecvt` and
  `0518_standard_locale_facets` were real regressions, now pass; the
  rest is the environmental cluster plus `0340`, `1044`, `1045`). The
  mini-suite of the 16 direct-run failures matches baseline exactly.
  NOT rerun after the final `InstructionHoldsRegister` quad-pair
  hardening: ARM mini-suite / full suite (spots `0492` + `coalstress.c`
  pass; the pair change only adds rejections).
- **x86-64**: full direct O2 corpus pass=1022 fail=11, and all 11
  (`0111 0114 0115 0118 0119 0120 0124 0128 0403 0474 0475`) verified
  failing identically on baseline. Done. i386 profile shares the code
  but was not separately tested.
- **risc_v32**: full exec corpus via `bazel-bin/riscv` interpreter,
  pass=973 fail=57; all 57 verified failing identically on baseline.
  Done (no dedicated suite exists for this target).
- **Xtensa**: full exec corpus via `bazel-bin/esp32` interpreter
  (link recipe: `-nostdlib`, `bazel-bin/libc/esp32_start.o`,
  `-e _start`, `libcxtensa.a`), pass=667 fail=363. The baseline
  comparison of those 363 failures was **not finished** — this is the
  main remaining validation item. The backend is immature: float
  `printf`, float spill codegen (`s32i f0, ...` assembler error), and
  double support are broken *on baseline* too (verified individually
  for `coalstress.c` variants), so many failures are expected to be
  baseline noise. Spot tests `co.c`, `vfwd.c`, exec `0001` pass with
  the port; `1052`/`0473` fail and `0492` fails to compile with the
  port — baseline status unknown (covered by the unfinished
  comparison).

## Remaining work (priority order)

1. **Xtensa baseline comparison.** A mini-suite of exactly the 363
   ported-run failures is staged outside git at
   `/tmp/cg/xtsuite/tests/exec` (may not survive; regenerate from a
   ported full run if gone). Stash only
   `c_compiler/xtensa/xtensa_reg_alloc.c`, rebuild `//:davecc`, run
   the mini-suite with the xtensa command below, and confirm the
   baseline failure set is identical (every test fails on baseline
   too). Any test that *passes* on baseline is a real xtensa
   regression to debug. Note a previous attempt at this run aborted
   mid-way and left the port in `git stash` — it was restored with
   `git stash pop stash@{0}`; check `git stash list` before/after.
2. **ARM rerun after quad-pair hardening**: rerun the ARM mini-suite
   (`/tmp/cg/armsuite`, 16 tests) or the full ARM direct O2 suite and
   confirm the failure set still matches baseline.
3. **AArch64 suite rerun after the hardening scan**: full O2 and O0
   exec suites (commands below). Expected: O2 fail set = the 13-entry
   `exec_aarch64_O2.txt` baseline, O0 = `0065 0111 1044 1045`.
4. Optional: `bazel test //cxx_testsuite:exec_aarch64_O2
   //cxx_testsuite:exec_arm_O2 //cxx_testsuite:exec_riscv_O2
   //cxx_testsuite:exec_x86_64_O2` as an end-to-end confirmation
   (the bazel harness differs from the direct script; slower).
5. Optional: i386 spot check (`-target x86` + `libcx86.a` +
   `bazel-bin/x86_64` cannot run it — there is no i386 interpreter
   target wired up the same way; compile-only smoke plus the shared
   code path may be all that's practical).

## Reproducing the validation runs

Build: `perl -e 'alarm 300; exec @ARGV' bazel build //:davecc`
(wrap long commands in `perl -e 'alarm N; exec @ARGV'` throughout).

Direct exec suites (from repo root):

```sh
# AArch64 O2 (O0: drop -O2 and --expected-fail)
./cxx_testsuite/run_exec_tests.sh --davecc bazel-bin/davecc \
  --target aarch64 --libc bazel-bin/libc/libcaarch64.a \
  --interpreter bazel-bin/aarch64 \
  --compile-arg -Wl,-e --compile-arg -Wl,main --compile-arg -O2 \
  --interp-arg -i \
  --expected-fail cxx_testsuite/expected_fail/exec_aarch64_O2.txt \
  --suite-root cxx_testsuite

# ARM O2
./cxx_testsuite/run_exec_tests.sh --davecc bazel-bin/davecc \
  --target arm --libc bazel-bin/libc/libcarm.a \
  --interpreter bazel-bin/arm \
  --compile-arg -Wl,-e --compile-arg -Wl,main --compile-arg -O2 \
  --interp-arg -i --suite-root cxx_testsuite

# RISC-V O2
./cxx_testsuite/run_exec_tests.sh --davecc bazel-bin/davecc \
  --target riscv --libc bazel-bin/libc/libcriscv.a \
  --interpreter bazel-bin/riscv --compile-arg -O2 \
  --interp-arg -i --suite-root cxx_testsuite

# x86-64 O2
./cxx_testsuite/run_exec_tests.sh --davecc bazel-bin/davecc \
  --target x86_64 --libc bazel-bin/libc/libcx86_64.a \
  --interpreter bazel-bin/x86_64 \
  --compile-arg -Wl,-e --compile-arg -Wl,main --compile-arg -O2 \
  --interp-arg -i --suite-root cxx_testsuite

# riscv32 (full corpus; interpreter is the riscv binary, no -Wl flags)
./cxx_testsuite/run_exec_tests.sh --davecc bazel-bin/davecc \
  --target riscv32 --libc bazel-bin/libc/libcriscv32.a \
  --interpreter bazel-bin/riscv --compile-arg -O2 \
  --interp-arg -i --suite-root cxx_testsuite

# xtensa / esp32 (needs bazel build //:esp32 for the interpreter)
./cxx_testsuite/run_exec_tests.sh --davecc bazel-bin/davecc \
  --target esp32 --libc bazel-bin/libc/libcxtensa.a \
  --interpreter bazel-bin/esp32 \
  --compile-arg -nostdlib \
  --compile-arg bazel-bin/libc/esp32_start.o \
  --compile-arg -e --compile-arg _start --compile-arg -O2 \
  --suite-root cxx_testsuite
```

Per-test and C spot helpers used during development live (outside git)
in `/tmp/cg`: `one.sh`/`one_arm.sh`/`one_rv.sh`/`one_x64.sh`/
`one_rv32.sh`/`one_xt.sh NNNN` run one exec test by number prefix;
`run_c.sh`/`run_arm.sh`/`run_rv.sh`/`run_x64.sh`/`run_rv32.sh`/
`run_xt.sh file.c expected [opt]` compile-and-run a C file.
Adversarial C tests there: `co.c` (expects `441`), `coalstress.c`
(expects `261 8.0 308 4`; xtensa needs `coalstress_xt2.c`,
`261 80 308 4`, because float printf is broken on that backend),
`vfwd.c`. Mini-suites: `/tmp/cg/armsuite`, `/tmp/cg/rvsuite`,
`/tmp/cg/x64suite`, `/tmp/cg/rv32suite`, `/tmp/cg/xtsuite`.

BBC emulator (AArch64 performance/correctness):
`/tmp/bbc-davecc/build.sh OUT -O2`, then from `/tmp/bbc-davecc`:
`./cmp.sh /tmp/bbc-davecc/OUT 20000000` (want `ram same screen same`).

## History / debugging notes for whoever picks this up

- The ARM codecvt bug (`0492`) is the cautionary tale for this whole
  design: in `__decode_utf8`, the give path let variable X inherit `r8`
  from a dying constant in one block, but `r8` also held the byte value
  live into sibling blocks; a later write to X clobbered it because the
  allocator's per-block owner arbitration prefers the varreg's claim.
  Hence `RegisterHasOtherLiveHolder`. If a new miscompile appears in
  this area, bisect give vs. retarget first (temporarily return false
  from one path), then diff `-S` output function-by-function.
- The RISC-V regex bug was the same shape with a different register:
  the source load was allocated the return-address register `ra`
  (in the temp pool on RISC-V/RV32/Xtensa, unlike AArch64's x30), and
  the variable inheriting it died at the first call.
- `games/build/` is untracked build output; never commit it.
