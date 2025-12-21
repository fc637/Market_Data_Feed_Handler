#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT_DIR/build/feed_handler"

HOST=127.0.0.1
PORT=9876

if [[ ! -x "$BIN" ]]; then
  echo "[client] feed_handler not found. Run build.sh first."
  exit 1
fi

echo "[client] Connecting to $HOST:$PORT"
exec "$BIN" --host "$HOST" --port "$PORT"