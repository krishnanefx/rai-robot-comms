# Professor Server Contract To Confirm

This repo does not implement the professors' production server. The Arduino library
expects the official server to provide these capabilities, either directly or
through equivalent endpoints.

## Full Grid Fetch

Robot should be able to fetch all 81 cells at startup:

- coordinate: `A1` through `I9`
- fertility: `unknown`, `infertile`, `fertile`
- seed count
- blocked status, if available

If this fetch fails, the robot continues with all cells as `unknown`.

## Heartbeat Sync

Every robot sends a heartbeat every `250 ms` containing:

- robot ID
- current coordinate
- heading
- full planned route, or route ID plus current index plus skip list
- next coordinate/current target
- airlock intent
- wants-to-save flag
- seeds remaining
- disabled/stuck state

Server response should include:

- emergency return or software kill command
- airlock grant/deny/stuck warning
- whether Tunnel A or Tunnel B is stuck/unsafe
- rescue assignment, if any
- updated grid cells, if any
- other robot snapshots, if available

The client library treats Tunnel A as base entry and Tunnel B as base exit. A
robot requesting exit must first admit any live robot requesting entry.

## RFID Visit

Robot sends raw RFID UID. Server returns:

- coordinate
- fertility
- seed count
- blocked status, if available

## Planting Report

Robot reports that it planted at a coordinate. Server should accept or reject the
report and return the latest seed count if possible.

In this library, robot reports are trusted locally immediately, then newer server
data can overwrite them.

## Dashboard

The dashboard is currently mock-driven. Once the professor API is known, it should
poll the server every `250 ms` for:

- grid cells
- fleet snapshots
- airlock state
- event log/important match events
