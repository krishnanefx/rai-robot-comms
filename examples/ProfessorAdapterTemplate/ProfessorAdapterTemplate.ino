#include <RAIRobotComms.h>
#include "arduino_secrets.h"

using namespace rai;

// The professor API is not known yet. This adapter intentionally returns false
// until its HTTP endpoints and JSON payloads are confirmed.
ProfessorServerAdapter server(PROFESSOR_SERVER_URL);
RobotComms comms(server, ROBOT_ID);

RobotSnapshot status;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  copyId(status.robotId, sizeof(status.robotId), ROBOT_ID);
  status.current = GridCoord::fromLabel("A1");
  status.next = GridCoord::fromLabel("B1");
  status.heading = HEADING_E;
  status.routeLength = HamiltonianRoute::length();
  HamiltonianRoute::fill(status.route, status.routeLength);
  status.airlockIntent = AIRLOCK_NONE;
  status.wantsToSave = false;
  status.seedsRemaining = 5;
  status.mode = MODE_BASE;

  bool fetched = comms.begin(millis());
  Serial.print("Professor full-grid fetch: ");
  Serial.println(fetched ? "ok" : "not implemented yet");
}

void loop() {
  ServerCommand command;
  bool synced = comms.sync(status, millis(), &command);

  if (synced) {
    Serial.print("Server action: ");
    Serial.println(actionName(command.action));
  }

  delay(25);
}
