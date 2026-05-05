#include <RAIRobotComms.h>

using namespace rai;

MockServerAdapter server;
RobotComms comms(server, "TEAM_ASTRA");

RobotSnapshot status;
uint8_t routeIndex = 0;

void printCoord(GridCoord coord) {
  char label[4];
  coord.label(label, sizeof(label));
  Serial.print(label);
}

void setupStatus() {
  copyId(status.robotId, sizeof(status.robotId), "TEAM_ASTRA");
  status.heading = HEADING_N;
  status.airlockIntent = AIRLOCK_NONE;
  status.wantsToSave = false;
  status.seedsRemaining = 5;
  status.mode = MODE_EXPLORING;
  status.routeLength = HamiltonianRoute::length();
  HamiltonianRoute::fill(status.route, status.routeLength);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  setupStatus();

  bool fetched = comms.begin(millis());
  Serial.print("Full grid fetch: ");
  Serial.println(fetched ? "ok" : "fallback unknown");
}

void loop() {
  uint32_t now = millis();

  status.current = HamiltonianRoute::at(routeIndex);
  status.next = HamiltonianRoute::at((routeIndex + 1) % HamiltonianRoute::length());
  status.heading = (routeIndex % 2 == 0) ? HEADING_E : HEADING_W;

  GridCoord target;
  if (comms.grid().nextPlantingTarget(routeIndex, &target)) {
    status.next = target;
    comms.grid().reserve(target, status.robotId, now);
  }

  ServerCommand command;
  if (comms.sync(status, now, &command)) {
    Serial.print("Sync at ");
    Serial.print(now);
    Serial.print(" ms | current ");
    printCoord(status.current);
    Serial.print(" | next ");
    printCoord(status.next);
    Serial.print(" | action ");
    Serial.println(actionName(command.action));

    if (command.action == ACTION_EMERGENCY_RETURN || command.action == ACTION_SOFTWARE_KILL) {
      status.mode = MODE_RETURNING;
    }
  }

  if (routeIndex % 10 == 0) {
    VisitResult visit = comms.reportVisit("DE AD BE EF", now);
    if (visit.ok) {
      Serial.print("Visited ");
      printCoord(visit.coord);
      Serial.print(" | fertility ");
      Serial.print(fertilityName(visit.fertility));
      Serial.print(" | seeds ");
      Serial.println(visit.seedCount);
    }
  }

  if (comms.grid().isPlantable(status.current) && status.seedsRemaining > 0) {
    comms.reportPlanting(status.current, now);
    status.seedsRemaining--;
    Serial.print("Planted at ");
    printCoord(status.current);
    Serial.print(" | seeds remaining ");
    Serial.println(status.seedsRemaining);
  }

  routeIndex = (routeIndex + 1) % HamiltonianRoute::length();
  delay(250);
}
