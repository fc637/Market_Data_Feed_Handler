#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "[build] Root:  $ROOT_DIR"
echo "[build] Build: $BUILD_DIR"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "[build] Running cmake..."
cmake ..

echo "[build] Building..."
cmake --build . -- -j$(nproc)

echo "[build] Done"
