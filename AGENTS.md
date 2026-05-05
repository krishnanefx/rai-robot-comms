# Coding Agent Instructions

This repo is meant to be read by coding agents helping students add shared
communications to their own Arduino robot sketches. Do not replace a student's
navigation, motor, RFID, planter, or sensor code. Integrate this library around
their existing robot state machine.

## Goal

Make the student's robot publish useful shared state every `250 ms`, update its
local grid when it reads RFID tags or plants seeds, and obey server commands
such as emergency return, software kill, rescue assignment, and airlock warnings.

## Required Sketch Integration

In the student's main `.ino`:

1. Include the library:

   ```cpp
   #include <RAIRobotComms.h>
   using namespace rai;
   ```

2. Create exactly one adapter and one `RobotComms` instance near the top of the
   sketch.

   Use `MockServerAdapter` while testing without the professor server:

   ```cpp
   MockServerAdapter server;
   RobotComms comms(server, "TEAM_ID");
   ```

   Use `ProfessorServerAdapter` only after the real API details are implemented:

   ```cpp
   ProfessorServerAdapter server(PROFESSOR_SERVER_URL);
   RobotComms comms(server, ROBOT_ID);
   ```

3. Create one persistent `RobotSnapshot status;`. This is the data the robot
   publishes to the server/fleet.

4. In `setup()`, initialise all required fields once:

   ```cpp
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
   ```

5. In every `loop()`, before calling `comms.sync(...)`, update `status` from the
   student's real robot state:

   - `status.current`: latest known grid coordinate, usually from RFID/server.
   - `status.heading`: cardinal heading from line/wall/magnetometer/dead reckoning.
   - `status.next`: next grid coordinate the robot intends to visit.
   - `status.route`: full planned route, or the current route helper output.
   - `status.airlockIntent`: `AIRLOCK_NONE`, `AIRLOCK_ENTER_BASE`, or `AIRLOCK_EXIT_BASE`.
   - `status.wantsToSave`: true only if the robot is willing and able to rescue.
   - `status.seedsRemaining`: current onboard seed count.
   - `status.mode`: `MODE_BASE`, `MODE_EXPLORING`, `MODE_PLANTING`,
     `MODE_RETURNING`, `MODE_RESCUING`, `MODE_DISABLED`, or `MODE_STUCK`.

6. Call `comms.sync(status, millis(), &command)` every loop. The library itself
   throttles syncs to `250 ms`; do not add long blocking delays around it.

7. Immediately handle `ServerCommand`:

   - `ACTION_SOFTWARE_KILL`: stop motors and enter disabled state.
   - `ACTION_EMERGENCY_RETURN`: stop exploring/planting and return to base.
   - `ACTION_RESCUE_ASSIGNED`: navigate toward `command.target` if valid.
   - `ACTION_AIRLOCK_GRANTED`: proceed according to the robot's airlock state.
   - `ACTION_AIRLOCK_DENIED`: wait outside the airlock.
   - `ACTION_AIRLOCK_STUCK_WARNING`: do not enter the warned airlock.

8. When RFID is read, pass the raw UID string to `comms.reportVisit(uid, millis())`.
   Use the returned `VisitResult` to update the robot's `status.current` and decide
   whether the cell is fertile enough to plant.

9. When the planter successfully drops a seed, call
   `comms.reportPlanting(status.current, millis())` and decrement the student's
   local seed counter.

10. When choosing a planting target, prefer:

    ```cpp
    GridCoord target;
    if (comms.grid().nextPlantingTarget(routeIndex, &target)) {
      status.next = target;
      comms.grid().reserve(target, status.robotId, millis());
    }
    ```

## Do Not Do

- Do not implement a production server in this repo.
- Do not guess professor-server endpoints or JSON payloads. Put all real API work
  inside `ProfessorServerAdapter` after the API is confirmed.
- Do not make `loop()` block for seconds. Heartbeat freshness matters.
- Do not overwrite a student's motor safety logic, hardware kill switch, or
  existing navigation code.
- Do not treat raw RFID UID as a coordinate locally when using the professor
  server. The professor server resolves UID to coordinate.

## Files To Read First

- `docs/arduino-integration-guide.md`
- `src/RAIRobotComms.h`
- `examples/MockRobot/MockRobot.ino`
- `examples/ProfessorAdapterTemplate/ProfessorAdapterTemplate.ino`

