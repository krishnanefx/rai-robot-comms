#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

TEST_TMP_DIR="${TMPDIR:-/tmp}/rai-robot-comms-tests"
mkdir -p "$TEST_TMP_DIR"

c++ -std=c++11 -Wall -Wextra -Werror \
  tests/world_model_test.cpp src/RAIRobotComms.cpp \
  -o "$TEST_TMP_DIR/world_model_test"

"$TEST_TMP_DIR/world_model_test"
