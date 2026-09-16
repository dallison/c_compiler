#!/usr/bin/env bash
#
# Run a WASI command produced by davecc -target wasm32.
#
# Preopen the host cwd as guest / so relative fopen() and getcwd() look like
# a normal POSIX process, and /tmp so tests that name a path there can use it.
#
set -euo pipefail

if ! command -v wasmtime >/dev/null 2>&1; then
  PATH="/opt/homebrew/bin:/usr/local/bin:/opt/local/bin:$PATH"
fi
if ! command -v wasmtime >/dev/null 2>&1; then
  echo "wasm32run: wasmtime is not installed" >&2
  exit 127
fi

# Inherit the variables a POSIX program typically looks at.  Wasmtime does
# not forward the host environment unless named.
env_args=()
for var in PATH HOME USER TMPDIR LANG; do
  if eval "[ -n \"\${$var:-}\" ]"; then
    env_args+=(--env "$var")
  fi
done

exec wasmtime run "${env_args[@]}" --dir "${PWD}::/" --dir /tmp::/tmp "$@"
