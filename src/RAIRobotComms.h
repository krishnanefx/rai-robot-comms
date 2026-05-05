#ifndef RAI_ROBOT_COMMS_H
#define RAI_ROBOT_COMMS_H

#include <stdint.h>
#include <stddef.h>

#ifndef RAI_MAX_ROBOTS
#define RAI_MAX_ROBOTS 20
#endif

#ifndef RAI_MAX_ROUTE_POINTS
#define RAI_MAX_ROUTE_POINTS 81
#endif

#ifndef RAI_TARGET_SEEDS_PER_FERTILE_CELL
#define RAI_TARGET_SEEDS_PER_FERTILE_CELL 2
#endif

#ifndef RAI_HEARTBEAT_MS
#define RAI_HEARTBEAT_MS 250UL
#endif

#ifndef RAI_STALE_ROBOT_MS
#define RAI_STALE_ROBOT_MS 2000UL
#endif

namespace rai {

enum Fertility : uint8_t {
  FERTILITY_UNKNOWN = 0,
  FERTILITY_INFERTILE = 1,
  FERTILITY_FERTILE = 2
};

enum Heading : uint8_t {
  HEADING_N = 0,
  HEADING_E = 1,
  HEADING_S = 2,
  HEADING_W = 3
};

enum AirlockIntent : uint8_t {
  AIRLOCK_NONE = 0,
  AIRLOCK_ENTER_BASE = 1,
  AIRLOCK_EXIT_BASE = 2
};

enum RobotMode : uint8_t {
  MODE_BASE = 0,
  MODE_EXPLORING = 1,
  MODE_PLANTING = 2,
  MODE_RETURNING = 3,
  MODE_RESCUING = 4,
  MODE_DISABLED = 5,
  MODE_STUCK = 6
};

enum ServerAction : uint8_t {
  ACTION_NONE = 0,
  ACTION_SOFTWARE_KILL = 1,
  ACTION_EMERGENCY_RETURN = 2,
  ACTION_RESCUE_ASSIGNED = 3,
  ACTION_AIRLOCK_GRANTED = 4,
  ACTION_AIRLOCK_DENIED = 5,
  ACTION_AIRLOCK_STUCK_WARNING = 6
};

struct GridCoord {
  uint8_t col;
  uint8_t row;

  GridCoord();
  GridCoord(uint8_t colValue, uint8_t rowValue);

  bool isValid() const;
  uint8_t index() const;
  bool equals(const GridCoord &other) const;
  void label(char *buffer, size_t bufferSize) const;

  static GridCoord fromIndex(uint8_t indexValue);
  static GridCoord fromLabel(const char *label);
};

struct GridCell {
  GridCoord coord;
  Fertility fertility;
  uint8_t seedCount;
  bool blocked;
  char occupiedBy[16];
  char reservedBy[16];
  uint32_t lastUpdated;

  bool isPlantable(uint8_t targetSeeds = RAI_TARGET_SEEDS_PER_FERTILE_CELL) const;
  bool isFull(uint8_t targetSeeds = RAI_TARGET_SEEDS_PER_FERTILE_CELL) const;
  bool hasOccupant() const;
  bool hasReservation() const;
};

struct RobotSnapshot {
  char robotId[16];
  GridCoord current;
  Heading heading;
  GridCoord next;
  GridCoord route[RAI_MAX_ROUTE_POINTS];
  uint8_t routeLength;
  AirlockIntent airlockIntent;
  bool wantsToSave;
  uint8_t seedsRemaining;
  RobotMode mode;
  uint32_t lastHeartbeat;
};

struct ServerCommand {
  ServerAction action;
  char assignedRobotId[16];
  GridCoord target;
  bool emergencyActive;
  bool airlockAStuck;
  bool airlockBStuck;
};

struct VisitResult {
  bool ok;
  GridCoord coord;
  Fertility fertility;
  uint8_t seedCount;
  bool blocked;
};

class HamiltonianRoute {
public:
  static uint8_t length();
  static GridCoord at(uint8_t indexValue);
  static uint8_t indexOf(GridCoord coord);
  static void fill(GridCoord *buffer, uint8_t capacity);
};

class WorldGrid {
public:
  WorldGrid();

  void reset(uint32_t now = 0);
  GridCell *cell(GridCoord coord);
  const GridCell *cell(GridCoord coord) const;
  GridCell *cellByLabel(const char *label);

  bool updateCell(GridCoord coord, Fertility fertility, uint8_t seedCount, bool blocked, uint32_t timestamp);
  bool applyVisit(const VisitResult &visit, uint32_t timestamp);
  bool reportPlanting(GridCoord coord, const char *robotId, uint32_t timestamp);
  bool reserve(GridCoord coord, const char *robotId, uint32_t timestamp);
  bool occupy(GridCoord coord, const char *robotId, uint32_t timestamp);
  void clearRobotClaims(const char *robotId);
  void expireClaims(uint32_t now, uint32_t staleAfterMs = RAI_STALE_ROBOT_MS);

  bool isPlantable(GridCoord coord, uint8_t targetSeeds = RAI_TARGET_SEEDS_PER_FERTILE_CELL) const;
  bool nextPlantingTarget(uint8_t startRouteIndex, GridCoord *target, uint8_t targetSeeds = RAI_TARGET_SEEDS_PER_FERTILE_CELL) const;
  uint8_t fertileTargetsRemaining(uint8_t targetSeeds = RAI_TARGET_SEEDS_PER_FERTILE_CELL) const;
  uint16_t totalSeedsPlanted() const;

private:
  GridCell cells_[81];
};

class FleetState {
public:
  FleetState();

  void reset();
  bool upsert(const RobotSnapshot &snapshot);
  RobotSnapshot *find(const char *robotId);
  const RobotSnapshot *find(const char *robotId) const;
  void expire(uint32_t now, uint32_t staleAfterMs = RAI_STALE_ROBOT_MS);
  uint8_t count() const;
  uint8_t activeOnField() const;
  const RobotSnapshot *at(uint8_t indexValue) const;

private:
  RobotSnapshot robots_[RAI_MAX_ROBOTS];
  uint8_t count_;
};

class ServerAdapter {
public:
  virtual ~ServerAdapter() {}
  virtual bool fetchFullGrid(WorldGrid &grid, uint32_t now) = 0;
  virtual bool sync(const RobotSnapshot &status, WorldGrid &grid, FleetState &fleet, ServerCommand &command, uint32_t now) = 0;
  virtual VisitResult reportVisit(const char *rfidUid, uint32_t now) = 0;
  virtual bool reportPlanting(GridCoord coord, const char *robotId, uint32_t now) = 0;
};

class MockServerAdapter : public ServerAdapter {
public:
  MockServerAdapter();
  bool fetchFullGrid(WorldGrid &grid, uint32_t now) override;
  bool sync(const RobotSnapshot &status, WorldGrid &grid, FleetState &fleet, ServerCommand &command, uint32_t now) override;
  VisitResult reportVisit(const char *rfidUid, uint32_t now) override;
  bool reportPlanting(GridCoord coord, const char *robotId, uint32_t now) override;

private:
  WorldGrid serverGrid_;
  FleetState serverFleet_;
  bool initialized_;
};

class ProfessorServerAdapter : public ServerAdapter {
public:
  ProfessorServerAdapter(const char *serverUrl);
  bool fetchFullGrid(WorldGrid &grid, uint32_t now) override;
  bool sync(const RobotSnapshot &status, WorldGrid &grid, FleetState &fleet, ServerCommand &command, uint32_t now) override;
  VisitResult reportVisit(const char *rfidUid, uint32_t now) override;
  bool reportPlanting(GridCoord coord, const char *robotId, uint32_t now) override;

private:
  const char *serverUrl_;
};

class RobotComms {
public:
  RobotComms(ServerAdapter &adapter, const char *robotId);

  bool begin(uint32_t now);
  bool sync(const RobotSnapshot &status, uint32_t now, ServerCommand *command = 0);
  VisitResult reportVisit(const char *rfidUid, uint32_t now);
  bool reportPlanting(GridCoord coord, uint32_t now);

  WorldGrid &grid();
  FleetState &fleet();

private:
  ServerAdapter &adapter_;
  char robotId_[16];
  WorldGrid grid_;
  FleetState fleet_;
  uint32_t lastSync_;
};

void copyId(char *dest, size_t destSize, const char *source);
bool idEquals(const char *a, const char *b);
const char *fertilityName(Fertility fertility);
const char *headingName(Heading heading);
const char *actionName(ServerAction action);

}

#endif
