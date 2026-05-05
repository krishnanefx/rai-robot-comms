# Architecture

The repo has three layers.

## 1. Robot Library

Location: `src/`

Runs on Arduino. Owns:

- 9x9 grid model
- fleet snapshots
- default serpentine route
- planting target helper
- server adapter interface

It must stay independent of any team's motor, sensor, or navigation code.

## 2. Server Adapter Boundary

Location: `src/RAIRobotComms.cpp`

Adapters translate external communication into library types:

- `MockServerAdapter`: deterministic fake server for local robot testing.
- `ProfessorServerAdapter`: official API boundary, currently a stub.

When staff publish the API, implement WiFi, HTTP/WebSocket, JSON parsing, and
server payload mapping inside `ProfessorServerAdapter`. Do not put endpoint
details in student sketches.

## 3. Dashboard

Location: `dashboard/`

Runs in a browser. Owns:

- visual playfield
- robot list
- route overlays
- cell and robot detail panels
- event log

The UI consumes `PlayfieldState` from `dashboard/src/dataSource.ts`. Today that
state comes from mock data. Later it should come from professor-server polling.

## Data Flow

```text
Student robot sketch
  -> updates RobotSnapshot from real sensors/state
  -> RobotComms.sync(...)
  -> ServerAdapter
  -> professor server or mock server
  -> WorldGrid + FleetState + ServerCommand
  -> student sketch handles command
```

Dashboard:

```text
Professor server or mock data
  -> dashboard data source
  -> PlayfieldState
  -> React control panel
```

## Design Rules

- Server data wins when it is newer.
- Robot planting reports are trusted locally immediately.
- A fertile cell is complete at `2` seeds by default.
- Robots publish state every `250 ms`.
- Robot reservations/occupancy are considered stale after `2 seconds`.
- Unknown cells remain visitable.
- A robot inside base must admit waiting `AIRLOCK_ENTER_BASE` robots before it
  requests `AIRLOCK_EXIT_BASE`.
- Robots must not enter an airlock when the server marks that airlock as stuck.
- Do not build a production server in this repo.
