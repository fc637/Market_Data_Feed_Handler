#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT_DIR/build/exchange_simulator"

PORT=9876

if [[ ! -x "$BIN" ]]; then
  echo "[server] exchange_simulator not found. Run build.sh first."
  exit 1
fi

echo "[server] Starting exchange simulator on port $PORT"
exec "$BIN" --port "$PORT"
