# Arduino Integration Guide

Use this guide when adding RAI robot communications to a student's Arduino GIGA
R1 WiFi sketch.

The library does not drive the robot. It only manages shared world state and
server communication. The student's existing code remains responsible for:

- motor control
- line/wall following
- RFID hardware reads
- heading estimation
- planter hardware
- revive button and LED hardware
- physical and software kill behavior

## Minimum Integration Skeleton

```cpp
#include <RAIRobotComms.h>
#include "arduino_secrets.h"

using namespace rai;

MockServerAdapter server; // swap for ProfessorServerAdapter after API is known
RobotComms comms(server, ROBOT_ID);
RobotSnapshot status;
ServerCommand command;

uint8_t routeIndex = 0;

void setupComms() {
  copyId(status.robotId, sizeof(status.robotId), ROBOT_ID);
  status.current = GridCoord::fromLabel("A1");
  status.next = GridCoord::fromLabel("B1");
  status.heading = HEADING_N;
  status.routeLength = HamiltonianRoute::length();
  HamiltonianRoute::fill(status.route, status.routeLength);
  status.airlockIntent = AIRLOCK_NONE;
  status.wantsToSave = false;
  status.seedsRemaining = 5;
  status.mode = MODE_BASE;

  comms.begin(millis());
}

void updateCommsStatusFromRobot() {
  // Replace these assignments with the student's real robot state.
  status.current = latestKnownGridCoordinate();
  status.heading = latestHeading();
  status.next = nextGridCoordinate();
  status.airlockIntent = currentAirlockIntent();
  status.wantsToSave = robotCanRescue();
  status.seedsRemaining = seedsRemaining();
  status.mode = currentRobotMode();
}

void handleServerCommand(const ServerCommand &command) {
  if (command.action == ACTION_SOFTWARE_KILL) {
    stopMotorsImmediately();
    status.mode = MODE_DISABLED;
  } else if (command.action == ACTION_EMERGENCY_RETURN) {
    beginReturnToBase();
    status.mode = MODE_RETURNING;
  } else if (command.action == ACTION_RESCUE_ASSIGNED && command.target.isValid()) {
    beginRescue(command.target);
    status.mode = MODE_RESCUING;
  } else if (command.action == ACTION_AIRLOCK_STUCK_WARNING) {
    avoidUnsafeAirlocks(command.airlockAStuck, command.airlockBStuck);
  }
}

void loopComms() {
  updateCommsStatusFromRobot();
  if (comms.sync(status, millis(), &command)) {
    handleServerCommand(command);
  }
}
```

## What To Update In `status`

`RobotSnapshot status` is the one object that describes this robot to the
server and to other robots.

| Field | What to put in it |
| --- | --- |
| `robotId` | Stable readable team ID, for example `TEAM_ASTRA`. |
| `current` | Current grid coordinate. Update after RFID/server confirms location. |
| `heading` | `HEADING_N`, `HEADING_E`, `HEADING_S`, or `HEADING_W`. |
| `next` | Next grid coordinate the robot intends to visit. |
| `route` | Full planned route. Use `HamiltonianRoute::fill(...)` unless team has a better plan. |
| `routeLength` | Number of valid route points. Usually `81`. |
| `airlockIntent` | `AIRLOCK_NONE`, `AIRLOCK_ENTER_BASE`, or `AIRLOCK_EXIT_BASE`. |
| `wantsToSave` | True only when the robot is available for rescue. |
| `seedsRemaining` | Seeds still inside the robot. |
| `mode` | Current high-level state: base, exploring, planting, returning, rescuing, disabled, stuck. |

## RFID Visit Flow

When the RFID reader sees a tag:

```cpp
VisitResult visit = comms.reportVisit(uidString, millis());

if (visit.ok) {
  status.current = visit.coord;

  if (visit.fertility == FERTILITY_FERTILE &&
      visit.seedCount < RAI_TARGET_SEEDS_PER_FERTILE_CELL &&
      status.seedsRemaining > 0) {
    // Move to planting behavior if mechanically aligned with the hole.
    status.mode = MODE_PLANTING;
  }
}
```

Important: with the real professor server, send the raw UID. Do not hard-code
UID-to-coordinate mapping in robot sketches unless staff explicitly tell you to.

## Planting Flow

Only call `reportPlanting` after the mechanical planter has actually released a
seed.

```cpp
if (seedDropConfirmed()) {
  comms.reportPlanting(status.current, millis());
  if (status.seedsRemaining > 0) {
    status.seedsRemaining--;
  }
}
```

The local matrix trusts the report immediately. Newer server data can overwrite
the count later.

## Choosing The Next Planting Target

The default route is:

`A1 -> B1 -> ... -> I1 -> I2 -> H2 -> ... -> A2 -> A3 ... -> I9`

To select the next useful target in route order:

```cpp
GridCoord target;
if (comms.grid().nextPlantingTarget(routeIndex, &target)) {
  status.next = target;
  comms.grid().reserve(target, status.robotId, millis());
} else {
  status.next = HamiltonianRoute::at((routeIndex + 1) % HamiltonianRoute::length());
}
```

The helper skips cells that are:

- infertile
- already at 2 seeds
- blocked
- occupied
- reserved by another robot

Unknown cells remain visitable so the robot can discover them.

## Airlock Flow

Before entering or leaving an airlock:

1. Set `status.airlockIntent`.
2. Continue calling `comms.sync(...)`.
3. Wait for `ACTION_AIRLOCK_GRANTED`.
4. If `ACTION_AIRLOCK_DENIED`, wait safely outside.
5. If `ACTION_AIRLOCK_STUCK_WARNING`, avoid that airlock.

When leaving base, first let waiting robots into base if your robot is in a
position to admit them, then request exit.

## Rescue Flow

Set `status.wantsToSave = true` only when all of these are true:

- robot is mobile
- robot has enough time/battery to help
- robot is not carrying out a higher-priority emergency return
- robot can physically reach stranded robots

When the server sends `ACTION_RESCUE_ASSIGNED`, use `command.target` as the
target coordinate and switch the student's navigation state to rescue mode.

## Timing Rules

- Call `loopComms()` every main loop.
- `RobotComms` throttles network syncs to `250 ms`.
- Avoid blocking delays longer than a few tens of milliseconds in normal driving.
- If a long mechanical action is unavoidable, keep calling `loopComms()` inside
  that action's wait loop.

## Professor Server Adapter Work

Do not scatter server code through student sketches.

All real server work belongs in `ProfessorServerAdapter`:

- WiFi connection
- HTTP/WebSocket requests
- JSON parsing
- auth/token handling if required
- translating professor payloads into `WorldGrid`, `FleetState`, `VisitResult`,
  and `ServerCommand`

Student sketches should not need to change when the professor API details are
updated.

