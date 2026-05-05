# API Reference

This is the stable surface coding agents and student sketches should use.

## `RobotComms`

`RobotComms(ServerAdapter &adapter, const char *robotId)`

Creates the comms object. Use one instance per robot sketch.

`bool begin(uint32_t now)`

Attempts to fetch the full grid. If the adapter fails, the local grid is reset to
all `unknown` and the return value is `false`.

`bool sync(const RobotSnapshot &status, uint32_t now, ServerCommand *command)`

Publishes robot state and receives server/fleet updates. It is internally
throttled to `RAI_HEARTBEAT_MS` (`250 ms`). A `false` return can mean either
"not time to sync yet" or "adapter failed"; sketches should keep calling it.

`VisitResult reportVisit(const char *rfidUid, uint32_t now)`

Sends a raw RFID UID to the adapter and applies the returned cell state to the
local grid.

`bool reportPlanting(GridCoord coord, uint32_t now)`

Reports a planted seed. The local grid increments immediately; newer server data
can overwrite it later.

## `RobotSnapshot`

The heartbeat payload:

- `robotId`: readable stable team ID.
- `current`: current coordinate.
- `heading`: `HEADING_N`, `HEADING_E`, `HEADING_S`, or `HEADING_W`.
- `next`: next target coordinate.
- `route`: planned route.
- `routeLength`: number of valid route entries.
- `airlockIntent`: no airlock, enter base, or exit base.
- `wantsToSave`: whether this robot is available for rescue.
- `seedsRemaining`: onboard seed count.
- `mode`: base, exploring, planting, returning, rescuing, disabled, or stuck.

## `WorldGrid`

Stores all 81 cells. Main helpers:

- `cell(coord)`: get a mutable cell pointer.
- `updateCell(...)`: apply newest-timestamp-wins cell update.
- `reportPlanting(...)`: trust a robot planting report locally.
- `reserve(...)`: reserve a target cell for one robot.
- `occupy(...)`: mark where a robot currently is.
- `expireClaims(...)`: clear stale occupancy/reservations.
- `isPlantable(...)`: true for fertile, not full, not blocked, not occupied, not reserved.
- `nextPlantingTarget(...)`: route-order target selection with dynamic skips.

## `FleetState`

Stores the latest snapshots for other robots.

- `count()`: number of stored snapshots.
- `activeOnField()`: count robots active outside base.
- `hasRobotWaitingToEnterBase(excludeRobotId)`: true when another live robot has
  `AIRLOCK_ENTER_BASE` intent.
- `firstRobotWaitingToEnterBase(excludeRobotId)`: first matching robot snapshot,
  useful before a base robot requests exit.
- `at(index)`: read a stored snapshot.

## `HamiltonianRoute`

The default shared route is serpentine, starting `A1 -> B1 -> ... -> I1`, then
`I2 -> H2 -> ... -> A2`, and so on.

- `length()`: always `81`.
- `at(index)`: route coordinate for an index.
- `indexOf(coord)`: route index for a coordinate.
- `fill(buffer, capacity)`: copy the route into a sketch-owned array.

## `ServerAdapter`

Abstract boundary for communication backends.

- `MockServerAdapter`: local deterministic fake server for testing.
- `ProfessorServerAdapter`: placeholder for the official server once its API is
  confirmed.

Do not expose HTTP/JSON details to student sketches.

## Airlock Helpers

`shouldAvoidAirlock(command, intent)`

Returns true when the latest server command says the airlock for that intent is
stuck or unsafe. Use it before driving into Tunnel A or Tunnel B.

Rule for base exit: if a robot in base is about to set `AIRLOCK_EXIT_BASE`, it
must first check `comms.fleet().hasRobotWaitingToEnterBase(status.robotId)`. If
true, admit that waiting robot before requesting exit.
