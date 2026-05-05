import type {
  AirlockInfo,
  EventItem,
  Fertility,
  GridCell,
  GridCoord,
  Heading,
  PlayfieldState,
  RobotMode,
  RobotState,
} from "./types";

const teams = [
  ["Astra", "#42d9ff"],
  ["Borealis", "#ffcf5a"],
  ["Cosmos", "#7cff9b"],
  ["Drift", "#ff7ad9"],
  ["Echo", "#9d8cff"],
  ["Flux", "#ff8b5c"],
  ["Gaia", "#55f0c2"],
  ["Helio", "#f36b7f"],
  ["Ion", "#b8f36b"],
  ["Juno", "#6ba7ff"],
  ["Kepler", "#ffef7a"],
  ["Luna", "#c88cff"],
  ["Mars", "#ff5959"],
  ["Nova", "#81e6ff"],
] as const;

const modes: RobotMode[] = ["exploring", "planting", "returning", "rescuing"];
const headings: Heading[] = ["N", "E", "S", "W"];

export function coord(col: number, row: number): GridCoord {
  return {
    col,
    row,
    label: `${String.fromCharCode(64 + col)}${row}`,
  };
}

function routeFrom(startOffset: number): GridCoord[] {
  const route: GridCoord[] = [];
  for (let row = 1; row <= 9; row += 1) {
    const cols = row % 2 === 1 ? [1, 2, 3, 4, 5, 6, 7, 8, 9] : [9, 8, 7, 6, 5, 4, 3, 2, 1];
    for (const col of cols) route.push(coord(col, row));
  }
  return [...route.slice(startOffset), ...route.slice(0, startOffset)];
}

function fertilityFor(col: number, row: number): Fertility {
  const v = (col * 7 + row * 11) % 10;
  if (v <= 1) return "unknown";
  if (v <= 4) return "infertile";
  return "fertile";
}

function seedCountFor(col: number, row: number, tick: number, fertility: Fertility): number {
  if (fertility !== "fertile") return 0;
  return (col + row + Math.floor(tick / 12)) % 3;
}

export function createPlayfieldState(tick: number): PlayfieldState {
  const now = Date.now();
  const robots = createRobots(tick, now);
  const cells = createCells(tick, robots, now);
  const airlocks = createAirlocks(tick);

  return {
    cells,
    robots,
    airlocks,
    events: createEvents(tick, robots),
    emergency: tick % 90 > 74,
    tick,
  };
}

function createRobots(tick: number, now: number): RobotState[] {
  return teams.map(([team, color], index) => {
    const fullRoute = routeFrom((index * 6) % 81);
    const routeIndex = (tick + index * 5) % fullRoute.length;
    const activeRoute = [...fullRoute.slice(routeIndex), ...fullRoute.slice(0, routeIndex)].slice(0, 18);
    const stale = index === 10 && tick % 70 > 52;

    return {
      id: `R${String(index + 1).padStart(2, "0")}`,
      team,
      color,
      coord: activeRoute[0],
      target: activeRoute[1],
      route: activeRoute,
      heading: headings[(tick + index) % headings.length],
      seedsRemaining: Math.max(0, 5 - ((tick + index) % 6)),
      mode: stale ? "stale" : modes[(tick + index) % modes.length],
      wantsToSave: (tick + index) % 7 === 0,
      lastHeartbeat: now - (stale ? 4800 : 120 + ((index * 37) % 160)),
    };
  });
}

function createCells(tick: number, robots: RobotState[], now: number): GridCell[] {
  const cells: GridCell[] = [];
  for (let row = 9; row >= 1; row -= 1) {
    for (let col = 1; col <= 9; col += 1) {
      const fertility = fertilityFor(col, row);
      const occupyingRobot = robots.find((robot) => robot.coord.col === col && robot.coord.row === row && robot.mode !== "stale");
      const reservingRobot = robots.find((robot) => robot.target.col === col && robot.target.row === row && robot.mode !== "stale");
      cells.push({
        ...coord(col, row),
        fertility,
        seedCount: seedCountFor(col, row, tick, fertility),
        occupiedBy: occupyingRobot?.id,
        reservedBy: reservingRobot?.id,
        lastUpdated: now - ((col * row * 173) % 9000),
      });
    }
  }
  return cells;
}

function createAirlocks(tick: number): AirlockInfo[] {
  return [
    {
      id: "A",
      label: "Tunnel A",
      role: "entry",
      state: tick % 50 > 42 ? "occupied" : "queueing",
      queue: tick % 50 > 42 ? ["R04"] : ["R08", "R02", "R11"],
    },
    {
      id: "B",
      label: "Tunnel B",
      role: "exit",
      state: tick % 80 > 70 ? "stuck" : "clear",
      queue: tick % 80 > 70 ? ["R13"] : [],
    },
  ];
}

function createEvents(tick: number, robots: RobotState[]): EventItem[] {
  const robot = robots[tick % robots.length];
  const planted = robots[(tick + 4) % robots.length];
  return [
    {
      id: `event-${tick}-1`,
      time: "now",
      tone: tick % 90 > 74 ? "danger" : "info",
      message: tick % 90 > 74 ? "Emergency return warning active" : `${robot.id} updated route toward ${robot.target.label}`,
    },
    {
      id: `event-${tick}-2`,
      time: "12s",
      tone: "success",
      message: `${planted.id} reported seed planted at ${planted.coord.label}`,
    },
    {
      id: `event-${tick}-3`,
      time: "28s",
      tone: "warning",
      message: tick % 80 > 70 ? "Tunnel B warning: robot may be stuck" : "Tunnel A queue changed: 3 robots waiting",
    },
    {
      id: `event-${tick}-4`,
      time: "44s",
      tone: "info",
      message: "Full grid snapshot refreshed from professor server",
    },
  ];
}
