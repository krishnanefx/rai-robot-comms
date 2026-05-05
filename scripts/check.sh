#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "== Native world-model tests =="
bash "$ROOT_DIR/tests/run_tests.sh"

echo "== Dashboard install/build =="
cd "$ROOT_DIR/dashboard"
if [ -d node_modules ]; then
  npm run build
else
  npm ci
  npm run build
fi

echo "All checks passed."
