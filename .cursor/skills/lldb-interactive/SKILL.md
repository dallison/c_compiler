---
name: lldb-interactive
description: >-
  Drive lldb interactively across separate shell commands via a PTY-backed
  background server. Use when debugging native crashes, segfaults, flaky/
  intermittent failures, or when you need breakpoints, stepping, backtraces, or
  to re-run a program many times until it faults. Prefer this over one-shot
  `lldb -b -o run` because it keeps a live session and a real tty.
---

# Interactive lldb over a PTY

`scripts/lldb_pty.py` runs lldb inside a pseudo-terminal as a detached server so
you can debug interactively from ordinary one-shot shell calls (each `send`
returns just that command's output). This is the reliable way to catch flaky
native crashes: keep sending `run` until it faults, then inspect.

## Quick start

```bash
S=.cursor/skills/lldb-interactive/scripts/lldb_pty.py

# 1. Start a session on the target program (args after `--`).
python3 "$S" start --session bug -- ./bazel-bin/tool -c input

# 2. Run it (give a generous --timeout for slow/crashing programs).
python3 "$S" send --session bug --timeout 120 "run"

# 3. On a crash you land at the (lldb) prompt; inspect.
python3 "$S" send --session bug "bt 40"
python3 "$S" send --session bug "frame select 3"
python3 "$S" send --session bug "frame variable"
python3 "$S" send --session bug "p some_var"

# 4. Clean up.
python3 "$S" stop --session bug
```

## Catching a flaky crash

Loop `run` until the process stops with a signal instead of exiting cleanly:

```bash
for i in $(seq 1 20); do
  out=$(python3 "$S" send --session bug --timeout 120 "run")
  echo "$out"
  echo "$out" | grep -q "stop reason" && { echo "CRASHED on run $i"; break; }
  # lldb asks to restart; "y" is implied by issuing run again.
done
python3 "$S" send --session bug "bt 40"
```

`run` on an already-run target prompts "Do you want to restart? [Y/n]"; just
issue `run` again (the send writes it and reads the result). If output seems cut
off (program still producing output), drain more with:

```bash
python3 "$S" read --session bug --timeout 120
```

## Commands

| Command | Purpose |
|---------|---------|
| `start --session N -- <prog> [args]` | Launch lldb on `<prog>` in the background |
| `send --session N [--timeout S] [--idle S] "<cmd>"` | Run one lldb command, print its output |
| `read --session N [--timeout S]` | Drain output without sending a command |
| `stop --session N` | Kill the session |

- `--timeout` (default 30s): max wait for a command. Raise it for `run`/`continue`.
- `--idle` (default 0.4s): output is "done" after this much silence.
- Full transcript is also written to `/tmp/lldb_pty/<session>/log`.

## Tips

- Build the target with debug info (`-g`) for symbols. AddressSanitizer
  (`-fsanitize=address -g`) plus this runner pinpoints memory bugs: run under
  lldb and ASan halts at the faulting instruction with a report.
- Set breakpoints before `run`: `send --session N "b file.c:123"`.
- Use `--session` names to debug several programs at once.
