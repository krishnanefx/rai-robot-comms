#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
mkdir -p /private/tmp/rai-robot-comms-tests

c++ -std=c++11 -Wall -Wextra -Werror \
  tests/world_model_test.cpp src/RAIRobotComms.cpp \
  -o /private/tmp/rai-robot-comms-tests/world_model_test

/private/tmp/rai-robot-comms-tests/world_model_test
