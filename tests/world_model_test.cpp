#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/RAIRobotComms.h"

using namespace rai;

static void testCoordinates() {
  GridCoord a1 = GridCoord::fromLabel("A1");
  assert(a1.isValid());
  assert(a1.col == 1);
  assert(a1.row == 1);
  assert(GridCoord::fromLabel("I9").index() == 80);
  assert(!GridCoord::fromLabel("J1").isValid());
  assert(!GridCoord::fromLabel("A0").isValid());
  assert(!GridCoord::fromLabel("A10").isValid());
}

static void testRoute() {
  assert(HamiltonianRoute::length() == 81);
  assert(HamiltonianRoute::at(0).equals(GridCoord::fromLabel("A1")));
  assert(HamiltonianRoute::at(8).equals(GridCoord::fromLabel("I1")));
  assert(HamiltonianRoute::at(9).equals(GridCoord::fromLabel("I2")));
  assert(HamiltonianRoute::at(17).equals(GridCoord::fromLabel("A2")));
  assert(HamiltonianRoute::at(80).equals(GridCoord::fromLabel("I9")));
  assert(HamiltonianRoute::indexOf(GridCoord::fromLabel("I2")) == 9);
}

static void testGridPlantingAndSkips() {
  WorldGrid grid;
  grid.reset(100);
  grid.updateCell(GridCoord::fromLabel("A1"), FERTILITY_FERTILE, 0, false, 110);
  grid.updateCell(GridCoord::fromLabel("B1"), FERTILITY_INFERTILE, 0, false, 110);
  grid.updateCell(GridCoord::fromLabel("C1"), FERTILITY_FERTILE, 2, false, 110);
  grid.updateCell(GridCoord::fromLabel("D1"), FERTILITY_FERTILE, 1, false, 110);

  assert(grid.isPlantable(GridCoord::fromLabel("A1")));
  assert(!grid.isPlantable(GridCoord::fromLabel("B1")));
  assert(!grid.isPlantable(GridCoord::fromLabel("C1")));

  GridCoord target;
  assert(grid.nextPlantingTarget(0, &target));
  assert(target.equals(GridCoord::fromLabel("A1")));

  grid.reserve(GridCoord::fromLabel("A1"), "R01", 120);
  assert(grid.nextPlantingTarget(0, &target));
  assert(target.equals(GridCoord::fromLabel("D1")));

  grid.reportPlanting(GridCoord::fromLabel("D1"), "R02", 130);
  const GridCell *cell = grid.cell(GridCoord::fromLabel("D1"));
  assert(cell != 0);
  assert(cell->seedCount == 2);
  assert(!grid.isPlantable(GridCoord::fromLabel("D1")));
}

static void testNewestTimestampWins() {
  WorldGrid grid;
  grid.reset(100);
  assert(grid.updateCell(GridCoord::fromLabel("E5"), FERTILITY_FERTILE, 1, false, 200));
  assert(!grid.updateCell(GridCoord::fromLabel("E5"), FERTILITY_INFERTILE, 0, false, 199));
  const GridCell *cell = grid.cell(GridCoord::fromLabel("E5"));
  assert(cell != 0);
  assert(cell->fertility == FERTILITY_FERTILE);
  assert(cell->seedCount == 1);
}

static void testFleet() {
  FleetState fleet;
  RobotSnapshot snapshot = {};
  copyId(snapshot.robotId, sizeof(snapshot.robotId), "R01");
  snapshot.current = GridCoord::fromLabel("A1");
  snapshot.next = GridCoord::fromLabel("B1");
  snapshot.mode = MODE_EXPLORING;
  snapshot.lastHeartbeat = 1000;
  assert(fleet.upsert(snapshot));
  assert(fleet.count() == 1);
  assert(fleet.activeOnField() == 1);
  fleet.expire(4001);
  const RobotSnapshot *stored = fleet.find("R01");
  assert(stored != 0);
  assert(stored->mode == MODE_STUCK);
}

static void testRobotComms() {
  MockServerAdapter adapter;
  RobotComms comms(adapter, "R01");
  assert(comms.begin(0));

  RobotSnapshot status = {};
  copyId(status.robotId, sizeof(status.robotId), "R01");
  status.current = GridCoord::fromLabel("A1");
  status.next = GridCoord::fromLabel("B1");
  status.heading = HEADING_E;
  status.mode = MODE_EXPLORING;
  status.seedsRemaining = 5;
  HamiltonianRoute::fill(status.route, 81);
  status.routeLength = 81;

  ServerCommand command;
  assert(!comms.sync(status, 100, &command));
  assert(comms.sync(status, 260, &command));
  assert(comms.fleet().count() == 1);

  VisitResult visit = comms.reportVisit("DE AD BE EF", 300);
  assert(visit.ok);
  assert(visit.coord.isValid());
}

int main() {
  testCoordinates();
  testRoute();
  testGridPlantingAndSkips();
  testNewestTimestampWins();
  testFleet();
  testRobotComms();
  printf("world_model_test passed\n");
  return 0;
}
