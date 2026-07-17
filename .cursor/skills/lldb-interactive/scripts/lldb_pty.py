#!/usr/bin/env python3
"""Drive lldb interactively over a pseudo-terminal across shell invocations.

lldb needs a real tty to behave interactively.  This runs lldb inside a PTY as a
detached background server; separate `send`/`read` calls talk to it over a Unix
socket, so an agent can set breakpoints, step, inspect state, and re-run a flaky
program many times until it crashes -- all from ordinary one-shot shell commands.

Usage:
  lldb_pty.py start [--session NAME] [--lldb PATH] -- <program> [args...]
  lldb_pty.py send  [--session NAME] [--timeout S] [--idle S] "<lldb command>"
  lldb_pty.py read  [--session NAME] [--timeout S] [--idle S]
  lldb_pty.py stop  [--session NAME]

Notes:
  * `send` writes the command, then waits until output goes idle (or --timeout),
    and prints exactly the new output produced.  Use a large --timeout for
    commands that run the program (e.g. `run`) and may take a while to crash.
  * `read` drains any output that arrived since the last call without sending a
    command (useful after a `continue`/`run` that is still producing output).
  * Sessions are independent; default session name is "default".
"""

import argparse
import errno
import json
import os
import select
import signal
import socket
import sys
import threading
import time

STATE_ROOT = "/tmp/lldb_pty"


def session_dir(name):
    return os.path.join(STATE_ROOT, name)


def sock_path(name):
    return os.path.join(session_dir(name), "sock")


def log_path(name):
    return os.path.join(session_dir(name), "log")


# --------------------------------------------------------------------------
# Server
# --------------------------------------------------------------------------
class Server:
    def __init__(self, name, lldb_path, program_args):
        self.name = name
        self.lldb_path = lldb_path
        self.program_args = program_args
        self.log = bytearray()
        self.cursor = 0
        self.lock = threading.Lock()
        self.master_fd = None
        self.child_pid = None

    def spawn(self):
        import pty

        pid, master_fd = pty.fork()
        if pid == 0:
            # Child: exec lldb on the slave side of the pty.
            argv = [self.lldb_path, "--no-use-colors", "-X"]
            argv += ["--"] + self.program_args
            try:
                os.execvp(self.lldb_path, argv)
            except OSError as exc:  # pragma: no cover
                sys.stderr.write("exec lldb failed: %s\n" % exc)
                os._exit(127)
        self.child_pid = pid
        self.master_fd = master_fd

    def reader(self):
        logf = open(log_path(self.name), "wb", buffering=0)
        while True:
            try:
                r, _, _ = select.select([self.master_fd], [], [], 0.2)
            except (OSError, ValueError):
                break
            if not r:
                continue
            try:
                data = os.read(self.master_fd, 65536)
            except OSError:
                break
            if not data:
                break
            with self.lock:
                self.log += data
            try:
                logf.write(data)
            except OSError:
                pass

    def wait_idle(self, idle, timeout, expect_prompt):
        deadline = time.time() + timeout
        with self.lock:
            last_len = len(self.log)
        stable_since = time.time()
        while time.time() < deadline:
            time.sleep(0.1)
            with self.lock:
                cur = len(self.log)
                tail = bytes(self.log[self.cursor:])
            if cur != last_len:
                last_len = cur
                stable_since = time.time()
                continue
            if time.time() - stable_since >= idle:
                if not expect_prompt or tail.rstrip().endswith(b"(lldb)"):
                    return

    def new_output(self):
        with self.lock:
            out = bytes(self.log[self.cursor:])
            self.cursor = len(self.log)
        return out

    def handle(self, req):
        op = req.get("op")
        idle = float(req.get("idle", 0.4))
        timeout = float(req.get("timeout", 30))
        if op == "stop":
            return b"stopping"
        if op == "send":
            cmd = req.get("cmd", "")
            os.write(self.master_fd, (cmd + "\n").encode())
            self.wait_idle(idle, timeout, expect_prompt=True)
            return self.new_output()
        if op == "read":
            self.wait_idle(idle, timeout, expect_prompt=False)
            return self.new_output()
        return b"unknown op\n"

    def run(self):
        os.makedirs(session_dir(self.name), exist_ok=True)
        try:
            os.unlink(sock_path(self.name))
        except OSError:
            pass
        self.spawn()
        threading.Thread(target=self.reader, daemon=True).start()

        srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        srv.bind(sock_path(self.name))
        srv.listen(4)

        # Drain lldb's startup banner so the first `send` returns clean output.
        self.wait_idle(idle=0.5, timeout=15, expect_prompt=True)
        self.new_output()

        while True:
            conn, _ = srv.accept()
            try:
                buf = b""
                while b"\n" not in buf:
                    chunk = conn.recv(65536)
                    if not chunk:
                        break
                    buf += chunk
                if not buf:
                    conn.close()
                    continue
                req = json.loads(buf.split(b"\n", 1)[0].decode())
                out = self.handle(req)
                conn.sendall(out)
                conn.close()
            except Exception as exc:  # pragma: no cover
                try:
                    conn.sendall(("error: %s\n" % exc).encode())
                    conn.close()
                except OSError:
                    pass
            if req.get("op") == "stop":
                break

        try:
            os.kill(self.child_pid, signal.SIGKILL)
        except OSError:
            pass
        try:
            srv.close()
            os.unlink(sock_path(self.name))
        except OSError:
            pass


# --------------------------------------------------------------------------
# Client
# --------------------------------------------------------------------------
def client_request(name, req):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(sock_path(name))
    s.sendall((json.dumps(req) + "\n").encode())
    s.shutdown(socket.SHUT_WR)
    out = b""
    while True:
        chunk = s.recv(65536)
        if not chunk:
            break
        out += chunk
    s.close()
    return out


def do_start(args):
    if os.path.exists(sock_path(args.session)):
        # Stale socket?  Try to connect; if it works, refuse.
        try:
            client_request(args.session, {"op": "read", "timeout": 1, "idle": 0.2})
            sys.stderr.write("session '%s' already running; stop it first\n"
                             % args.session)
            return 1
        except OSError:
            try:
                os.unlink(sock_path(args.session))
            except OSError:
                pass

    if not args.program:
        sys.stderr.write("start requires: -- <program> [args...]\n")
        return 2

    pid = os.fork()
    if pid > 0:
        for _ in range(200):
            if os.path.exists(sock_path(args.session)):
                print("lldb session '%s' started (daemon pid %d)"
                      % (args.session, pid))
                return 0
            time.sleep(0.05)
        sys.stderr.write("timed out waiting for session to start\n")
        return 1

    # Daemon child.
    os.setsid()
    devnull = os.open(os.devnull, os.O_RDWR)
    os.dup2(devnull, 0)
    os.dup2(devnull, 1)
    os.dup2(devnull, 2)
    Server(args.session, args.lldb, args.program).run()
    os._exit(0)


def do_send(args):
    out = client_request(args.session, {
        "op": "send", "cmd": args.command,
        "timeout": args.timeout, "idle": args.idle,
    })
    sys.stdout.buffer.write(out)
    return 0


def do_read(args):
    out = client_request(args.session, {
        "op": "read", "timeout": args.timeout, "idle": args.idle,
    })
    sys.stdout.buffer.write(out)
    return 0


def do_stop(args):
    try:
        client_request(args.session, {"op": "stop", "timeout": 2, "idle": 0.2})
    except OSError:
        pass
    d = session_dir(args.session)
    for p in (sock_path(args.session),):
        try:
            os.unlink(p)
        except OSError:
            pass
    print("stopped session '%s'" % args.session)
    return 0


def main():
    p = argparse.ArgumentParser(description="Interactive lldb over a PTY.")
    sub = p.add_subparsers(dest="op", required=True)

    ps = sub.add_parser("start")
    ps.add_argument("--session", default="default")
    ps.add_argument("--lldb", default="lldb")
    ps.add_argument("program", nargs=argparse.REMAINDER)

    for opname in ("send", "read"):
        sp = sub.add_parser(opname)
        sp.add_argument("--session", default="default")
        sp.add_argument("--timeout", type=float, default=30.0)
        sp.add_argument("--idle", type=float, default=0.4)
        if opname == "send":
            sp.add_argument("command")

    pt = sub.add_parser("stop")
    pt.add_argument("--session", default="default")

    args = p.parse_args()
    if args.op == "start":
        # Strip a leading "--" separator from REMAINDER if present.
        if args.program and args.program[0] == "--":
            args.program = args.program[1:]
        return do_start(args)
    if args.op == "send":
        return do_send(args)
    if args.op == "read":
        return do_read(args)
    if args.op == "stop":
        return do_stop(args)
    return 2


if __name__ == "__main__":
    sys.exit(main())
