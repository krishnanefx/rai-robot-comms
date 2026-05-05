# RAI Robot Comms

Shared communication tooling for the 2026 RAI robotics challenge.

The repo is intentionally comms-first:

- Arduino IDE library for robot-side world state and server communication.
- No production server implementation.
- Professor-server API details isolated behind `ProfessorServerAdapter`.
- Mock adapter for local development before the official API is available.
- React dashboard for students to watch the playfield with live mock updates.

## Repo Layout

- `src/RAIRobotComms.h`: public Arduino library API.
- `src/RAIRobotComms.cpp`: grid, fleet, route, adapter, and comms implementation.
- `examples/MockRobot`: runnable Arduino mock example.
- `examples/ProfessorAdapterTemplate`: real-server integration template.
- `dashboard`: React/Vite live control panel.
- `tests`: native C++ tests for route, grid, fleet, and mock comms logic.
- `AGENTS.md`: direct instructions for coding agents modifying student sketches.
- `docs/arduino-integration-guide.md`: step-by-step Arduino integration guide.
- `docs/professor-api-contract.md`: API assumptions to confirm with staff.

## Robot-Side Model

The library stores a 9x9 matrix of `A1` through `I9`.

- Viewed from the base, `A-I` goes left to right.
- Viewed from the base, `1-9` goes outward.
- Headings are `N`, `E`, `S`, `W`; `N` means outward from the base.
- Fertility is `unknown`, `infertile`, or `fertile`.
- Seed count is trusted locally when the robot reports planting.
- Fertile cells are treated as complete at `2` seeds by default.

The shared route is serpentine:

`A1 -> B1 -> ... -> I1 -> I2 -> H2 -> ... -> A2 -> A3 ... -> I9`

This is a Hamiltonian-style path, not a strict 9x9 orthogonal cycle.

## Build And Test

Run dashboard checks:

```bash
cd dashboard
npm install
npm run build
```

Run native library tests:

```bash
tests/run_tests.sh
```

Run the dashboard:

```bash
cd dashboard
npm run dev
```

## For Coding Agents

If you are adding comms to a student's robot sketch, read `AGENTS.md` first.
Then follow `docs/arduino-integration-guide.md`. The short version:

- keep the student's robot/navigation code;
- add one adapter, one `RobotComms`, and one persistent `RobotSnapshot`;
- update `RobotSnapshot` from the robot's real state every loop;
- call `comms.sync(...)` continuously;
- call `reportVisit(...)` when RFID is read;
- call `reportPlanting(...)` only after the planter drops a seed;
- handle `ServerCommand` immediately for kill, emergency return, rescue, and airlock safety.
