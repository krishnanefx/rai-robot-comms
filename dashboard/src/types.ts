export type Fertility = "unknown" | "infertile" | "fertile";
export type Heading = "N" | "E" | "S" | "W";
export type RobotMode = "base" | "exploring" | "planting" | "returning" | "rescuing" | "stale";
export type AirlockState = "clear" | "queueing" | "occupied" | "stuck";

export interface GridCoord {
  col: number;
  row: number;
  label: string;
}

export interface GridCell extends GridCoord {
  fertility: Fertility;
  seedCount: number;
  occupiedBy?: string;
  reservedBy?: string;
  lastUpdated: number;
}

export interface RobotState {
  id: string;
  team: string;
  color: string;
  coord: GridCoord;
  heading: Heading;
  target: GridCoord;
  route: GridCoord[];
  seedsRemaining: number;
  mode: RobotMode;
  wantsToSave: boolean;
  lastHeartbeat: number;
}

export interface AirlockInfo {
  id: "A" | "B";
  label: string;
  role: "entry" | "exit";
  state: AirlockState;
  queue: string[];
}

export interface EventItem {
  id: string;
  time: string;
  tone: "info" | "success" | "warning" | "danger";
  message: string;
}

export interface PlayfieldState {
  cells: GridCell[];
  robots: RobotState[];
  airlocks: AirlockInfo[];
  events: EventItem[];
  emergency: boolean;
  tick: number;
}
