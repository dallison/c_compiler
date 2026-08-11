#!/bin/bash
set -euo pipefail

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
BIN="$ROOT/$1"
FIXTURES="${TEST_TMPDIR:-/tmp}/chrono-host-fixtures"
python3 "$ROOT/tests/generate_tzif_fixtures.py" "$FIXTURES"
export DAVE_TZDIR="$FIXTURES"
export TZ=FixedOffset
exec "$BIN"
