#include "RAIRobotComms.h"

#include <string.h>

namespace rai {

static bool emptyId(const char *value) {
  return value == 0 || value[0] == '\0';
}

void copyId(char *dest, size_t destSize, const char *source) {
  if (destSize == 0) return;
  if (source == 0) source = "";
  strncpy(dest, source, destSize - 1);
  dest[destSize - 1] = '\0';
}

bool idEquals(const char *a, const char *b) {
  if (a == 0 || b == 0) return false;
  return strncmp(a, b, 16) == 0;
}

GridCoord::GridCoord() : col(0), row(0) {}

GridCoord::GridCoord(uint8_t colValue, uint8_t rowValue) : col(colValue), row(rowValue) {}

bool GridCoord::isValid() const {
  return col >= 1 && col <= 9 && row >= 1 && row <= 9;
}

uint8_t GridCoord::index() const {
  if (!isValid()) return 255;
  return (row - 1) * 9 + (col - 1);
}

bool GridCoord::equals(const GridCoord &other) const {
  return col == other.col && row == other.row;
}

void GridCoord::label(char *buffer, size_t bufferSize) const {
  if (bufferSize == 0) return;
  if (!isValid() || bufferSize < 3) {
    buffer[0] = '\0';
    return;
  }
  buffer[0] = static_cast<char>('A' + col - 1);
  buffer[1] = static_cast<char>('0' + row);
  buffer[2] = '\0';
}

GridCoord GridCoord::fromIndex(uint8_t indexValue) {
  if (indexValue >= 81) return GridCoord();
  return GridCoord((indexValue % 9) + 1, (indexValue / 9) + 1);
}

GridCoord GridCoord::fromLabel(const char *label) {
  if (label == 0 || label[0] == '\0' || label[1] == '\0') return GridCoord();
  char letter = label[0];
  if (letter >= 'a' && letter <= 'i') letter = static_cast<char>(letter - 'a' + 'A');
  if (letter < 'A' || letter > 'I') return GridCoord();
  if (label[1] < '1' || label[1] > '9') return GridCoord();
  if (label[2] != '\0') return GridCoord();
  return GridCoord(static_cast<uint8_t>(letter - 'A' + 1), static_cast<uint8_t>(label[1] - '0'));
}

bool GridCell::isPlantable(uint8_t targetSeeds) const {
  return fertility == FERTILITY_FERTILE && seedCount < targetSeeds && !blocked && !hasOccupant() && !hasReservation();
}

bool GridCell::isFull(uint8_t targetSeeds) const {
  return fertility == FERTILITY_FERTILE && seedCount >= targetSeeds;
}

bool GridCell::hasOccupant() const {
  return occupiedBy[0] != '\0';
}

bool GridCell::hasReservation() const {
  return reservedBy[0] != '\0';
}

uint8_t HamiltonianRoute::length() {
  return 81;
}

GridCoord HamiltonianRoute::at(uint8_t indexValue) {
  if (indexValue >= 81) return GridCoord();
  uint8_t row = (indexValue / 9) + 1;
  uint8_t offset = indexValue % 9;
  uint8_t col = (row % 2 == 1) ? offset + 1 : 9 - offset;
  return GridCoord(col, row);
}

uint8_t HamiltonianRoute::indexOf(GridCoord coord) {
  if (!coord.isValid()) return 255;
  uint8_t rowOffset = (coord.row - 1) * 9;
  uint8_t colOffset = (coord.row % 2 == 1) ? coord.col - 1 : 9 - coord.col;
  return rowOffset + colOffset;
}

void HamiltonianRoute::fill(GridCoord *buffer, uint8_t capacity) {
  if (buffer == 0) return;
  uint8_t count = capacity < 81 ? capacity : 81;
  for (uint8_t i = 0; i < count; i++) {
    buffer[i] = at(i);
  }
}

WorldGrid::WorldGrid() {
  reset();
}

void WorldGrid::reset(uint32_t now) {
  for (uint8_t i = 0; i < 81; i++) {
    cells_[i].coord = GridCoord::fromIndex(i);
    cells_[i].fertility = FERTILITY_UNKNOWN;
    cells_[i].seedCount = 0;
    cells_[i].blocked = false;
    cells_[i].occupiedBy[0] = '\0';
    cells_[i].reservedBy[0] = '\0';
    cells_[i].lastUpdated = now;
  }
}

GridCell *WorldGrid::cell(GridCoord coord) {
  if (!coord.isValid()) return 0;
  return &cells_[coord.index()];
}

const GridCell *WorldGrid::cell(GridCoord coord) const {
  if (!coord.isValid()) return 0;
  return &cells_[coord.index()];
}

GridCell *WorldGrid::cellByLabel(const char *label) {
  return cell(GridCoord::fromLabel(label));
}

bool WorldGrid::updateCell(GridCoord coord, Fertility fertility, uint8_t seedCount, bool blocked, uint32_t timestamp) {
  GridCell *target = cell(coord);
  if (target == 0) return false;
  if (timestamp < target->lastUpdated) return false;
  target->fertility = fertility;
  target->seedCount = seedCount;
  target->blocked = blocked;
  target->lastUpdated = timestamp;
  return true;
}

bool WorldGrid::applyVisit(const VisitResult &visit, uint32_t timestamp) {
  if (!visit.ok) return false;
  return updateCell(visit.coord, visit.fertility, visit.seedCount, visit.blocked, timestamp);
}

bool WorldGrid::reportPlanting(GridCoord coord, const char *robotId, uint32_t timestamp) {
  GridCell *target = cell(coord);
  if (target == 0) return false;
  if (timestamp < target->lastUpdated) return false;
  if (target->fertility == FERTILITY_UNKNOWN) target->fertility = FERTILITY_FERTILE;
  target->seedCount += 1;
  target->lastUpdated = timestamp;
  if (!emptyId(robotId)) copyId(target->reservedBy, sizeof(target->reservedBy), robotId);
  return true;
}

bool WorldGrid::reserve(GridCoord coord, const char *robotId, uint32_t timestamp) {
  GridCell *target = cell(coord);
  if (target == 0 || emptyId(robotId)) return false;
  if (!emptyId(target->reservedBy) && !idEquals(target->reservedBy, robotId)) return false;
  copyId(target->reservedBy, sizeof(target->reservedBy), robotId);
  target->lastUpdated = timestamp;
  return true;
}

bool WorldGrid::occupy(GridCoord coord, const char *robotId, uint32_t timestamp) {
  GridCell *target = cell(coord);
  if (target == 0 || emptyId(robotId)) return false;
  copyId(target->occupiedBy, sizeof(target->occupiedBy), robotId);
  target->lastUpdated = timestamp;
  return true;
}

void WorldGrid::clearRobotClaims(const char *robotId) {
  if (emptyId(robotId)) return;
  for (uint8_t i = 0; i < 81; i++) {
    if (idEquals(cells_[i].occupiedBy, robotId)) cells_[i].occupiedBy[0] = '\0';
    if (idEquals(cells_[i].reservedBy, robotId)) cells_[i].reservedBy[0] = '\0';
  }
}

void WorldGrid::expireClaims(uint32_t now, uint32_t staleAfterMs) {
  for (uint8_t i = 0; i < 81; i++) {
    if (now - cells_[i].lastUpdated > staleAfterMs) {
      cells_[i].occupiedBy[0] = '\0';
      cells_[i].reservedBy[0] = '\0';
    }
  }
}

bool WorldGrid::isPlantable(GridCoord coord, uint8_t targetSeeds) const {
  const GridCell *target = cell(coord);
  return target != 0 && target->isPlantable(targetSeeds);
}

bool WorldGrid::nextPlantingTarget(uint8_t startRouteIndex, GridCoord *target, uint8_t targetSeeds) const {
  if (target == 0) return false;
  for (uint8_t offset = 0; offset < 81; offset++) {
    GridCoord candidate = HamiltonianRoute::at((startRouteIndex + offset) % 81);
    if (isPlantable(candidate, targetSeeds)) {
      *target = candidate;
      return true;
    }
  }
  return false;
}

uint8_t WorldGrid::fertileTargetsRemaining(uint8_t targetSeeds) const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < 81; i++) {
    if (cells_[i].fertility == FERTILITY_FERTILE && cells_[i].seedCount < targetSeeds && !cells_[i].blocked) {
      count++;
    }
  }
  return count;
}

uint16_t WorldGrid::totalSeedsPlanted() const {
  uint16_t total = 0;
  for (uint8_t i = 0; i < 81; i++) {
    total += cells_[i].seedCount;
  }
  return total;
}

FleetState::FleetState() {
  reset();
}

void FleetState::reset() {
  count_ = 0;
  for (uint8_t i = 0; i < RAI_MAX_ROBOTS; i++) {
    robots_[i].robotId[0] = '\0';
    robots_[i].routeLength = 0;
    robots_[i].lastHeartbeat = 0;
    robots_[i].mode = MODE_BASE;
  }
}

bool FleetState::upsert(const RobotSnapshot &snapshot) {
  RobotSnapshot *existing = find(snapshot.robotId);
  if (existing != 0) {
    *existing = snapshot;
    return true;
  }
  if (count_ >= RAI_MAX_ROBOTS) return false;
  robots_[count_] = snapshot;
  count_++;
  return true;
}

RobotSnapshot *FleetState::find(const char *robotId) {
  if (emptyId(robotId)) return 0;
  for (uint8_t i = 0; i < count_; i++) {
    if (idEquals(robots_[i].robotId, robotId)) return &robots_[i];
  }
  return 0;
}

const RobotSnapshot *FleetState::find(const char *robotId) const {
  if (emptyId(robotId)) return 0;
  for (uint8_t i = 0; i < count_; i++) {
    if (idEquals(robots_[i].robotId, robotId)) return &robots_[i];
  }
  return 0;
}

void FleetState::expire(uint32_t now, uint32_t staleAfterMs) {
  for (uint8_t i = 0; i < count_; i++) {
    if (now - robots_[i].lastHeartbeat > staleAfterMs && robots_[i].mode != MODE_DISABLED) {
      robots_[i].mode = MODE_STUCK;
    }
  }
}

uint8_t FleetState::count() const {
  return count_;
}

uint8_t FleetState::activeOnField() const {
  uint8_t total = 0;
  for (uint8_t i = 0; i < count_; i++) {
    if (robots_[i].mode != MODE_BASE && robots_[i].mode != MODE_DISABLED && robots_[i].mode != MODE_STUCK) total++;
  }
  return total;
}

const RobotSnapshot *FleetState::at(uint8_t indexValue) const {
  if (indexValue >= count_) return 0;
  return &robots_[indexValue];
}

MockServerAdapter::MockServerAdapter() : initialized_(false) {}

bool MockServerAdapter::fetchFullGrid(WorldGrid &grid, uint32_t now) {
  serverGrid_.reset(now);
  for (uint8_t i = 0; i < 81; i++) {
    GridCoord coord = GridCoord::fromIndex(i);
    uint8_t score = static_cast<uint8_t>((coord.col * 7 + coord.row * 11) % 10);
    Fertility fertility = score <= 1 ? FERTILITY_UNKNOWN : (score <= 4 ? FERTILITY_INFERTILE : FERTILITY_FERTILE);
    uint8_t seedCount = fertility == FERTILITY_FERTILE ? static_cast<uint8_t>((coord.col + coord.row) % 2) : 0;
    serverGrid_.updateCell(coord, fertility, seedCount, false, now);
  }
  grid = serverGrid_;
  initialized_ = true;
  return true;
}

bool MockServerAdapter::sync(const RobotSnapshot &status, WorldGrid &grid, FleetState &fleet, ServerCommand &command, uint32_t now) {
  if (!initialized_) fetchFullGrid(grid, now);
  serverFleet_.upsert(status);
  serverGrid_.clearRobotClaims(status.robotId);
  serverGrid_.occupy(status.current, status.robotId, now);
  serverGrid_.reserve(status.next, status.robotId, now);
  grid = serverGrid_;
  fleet = serverFleet_;
  fleet.expire(now);
  command.action = ACTION_NONE;
  command.assignedRobotId[0] = '\0';
  command.target = GridCoord();
  command.emergencyActive = (now / 30000UL) % 5 == 4;
  command.airlockAStuck = false;
  command.airlockBStuck = (now / 45000UL) % 7 == 6;
  if (command.emergencyActive) command.action = ACTION_EMERGENCY_RETURN;
  if (command.airlockBStuck) command.action = ACTION_AIRLOCK_STUCK_WARNING;
  return true;
}

VisitResult MockServerAdapter::reportVisit(const char *rfidUid, uint32_t now) {
  if (!initialized_) serverGrid_.reset(now);
  uint16_t hash = 0;
  if (rfidUid != 0) {
    for (const char *p = rfidUid; *p != '\0'; p++) hash = static_cast<uint16_t>(hash * 31 + static_cast<uint8_t>(*p));
  }
  GridCoord coord = GridCoord::fromIndex(static_cast<uint8_t>(hash % 81));
  GridCell *cell = serverGrid_.cell(coord);
  VisitResult result;
  result.ok = cell != 0;
  result.coord = coord;
  result.fertility = cell != 0 ? cell->fertility : FERTILITY_UNKNOWN;
  result.seedCount = cell != 0 ? cell->seedCount : 0;
  result.blocked = cell != 0 ? cell->blocked : false;
  return result;
}

bool MockServerAdapter::reportPlanting(GridCoord coord, const char *robotId, uint32_t now) {
  initialized_ = true;
  return serverGrid_.reportPlanting(coord, robotId, now);
}

ProfessorServerAdapter::ProfessorServerAdapter(const char *serverUrl) : serverUrl_(serverUrl) {}

bool ProfessorServerAdapter::fetchFullGrid(WorldGrid &, uint32_t) {
  (void)serverUrl_;
  return false;
}

bool ProfessorServerAdapter::sync(const RobotSnapshot &, WorldGrid &, FleetState &, ServerCommand &command, uint32_t) {
  command.action = ACTION_NONE;
  command.assignedRobotId[0] = '\0';
  command.target = GridCoord();
  command.emergencyActive = false;
  command.airlockAStuck = false;
  command.airlockBStuck = false;
  return false;
}

VisitResult ProfessorServerAdapter::reportVisit(const char *, uint32_t) {
  VisitResult result;
  result.ok = false;
  result.coord = GridCoord();
  result.fertility = FERTILITY_UNKNOWN;
  result.seedCount = 0;
  result.blocked = false;
  return result;
}

bool ProfessorServerAdapter::reportPlanting(GridCoord, const char *, uint32_t) {
  return false;
}

RobotComms::RobotComms(ServerAdapter &adapter, const char *robotId) : adapter_(adapter), lastSync_(0) {
  copyId(robotId_, sizeof(robotId_), robotId);
}

bool RobotComms::begin(uint32_t now) {
  lastSync_ = 0;
  bool ok = adapter_.fetchFullGrid(grid_, now);
  if (!ok) grid_.reset(now);
  return ok;
}

bool RobotComms::sync(const RobotSnapshot &status, uint32_t now, ServerCommand *command) {
  if (now - lastSync_ < RAI_HEARTBEAT_MS) return false;
  RobotSnapshot outgoing = status;
  if (emptyId(outgoing.robotId)) copyId(outgoing.robotId, sizeof(outgoing.robotId), robotId_);
  outgoing.lastHeartbeat = now;
  ServerCommand localCommand;
  bool ok = adapter_.sync(outgoing, grid_, fleet_, localCommand, now);
  lastSync_ = now;
  if (command != 0) *command = localCommand;
  return ok;
}

VisitResult RobotComms::reportVisit(const char *rfidUid, uint32_t now) {
  VisitResult result = adapter_.reportVisit(rfidUid, now);
  grid_.applyVisit(result, now);
  return result;
}

bool RobotComms::reportPlanting(GridCoord coord, uint32_t now) {
  bool ok = adapter_.reportPlanting(coord, robotId_, now);
  grid_.reportPlanting(coord, robotId_, now);
  return ok;
}

WorldGrid &RobotComms::grid() {
  return grid_;
}

FleetState &RobotComms::fleet() {
  return fleet_;
}

const char *fertilityName(Fertility fertility) {
  switch (fertility) {
    case FERTILITY_INFERTILE: return "infertile";
    case FERTILITY_FERTILE: return "fertile";
    default: return "unknown";
  }
}

const char *headingName(Heading heading) {
  switch (heading) {
    case HEADING_E: return "E";
    case HEADING_S: return "S";
    case HEADING_W: return "W";
    default: return "N";
  }
}

const char *actionName(ServerAction action) {
  switch (action) {
    case ACTION_SOFTWARE_KILL: return "software_kill";
    case ACTION_EMERGENCY_RETURN: return "emergency_return";
    case ACTION_RESCUE_ASSIGNED: return "rescue_assigned";
    case ACTION_AIRLOCK_GRANTED: return "airlock_granted";
    case ACTION_AIRLOCK_DENIED: return "airlock_denied";
    case ACTION_AIRLOCK_STUCK_WARNING: return "airlock_stuck_warning";
    default: return "none";
  }
}

}
