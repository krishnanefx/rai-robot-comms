import React, { useState } from "react";
import { createRoot } from "react-dom/client";
import { refreshMs, usePlayfieldState } from "./dataSource";
import type { GridCell, PlayfieldState, RobotState } from "./types";
import "./styles.css";

function App() {
  const [paused, setPaused] = useState(false);
  const [selectedRobotId, setSelectedRobotId] = useState("R01");
  const [selectedCell, setSelectedCell] = useState("E5");
  const [showRoutes, setShowRoutes] = useState(true);
  const [showInfertile, setShowInfertile] = useState(true);

  const state = usePlayfieldState(paused);
  const selectedRobot = state.robots.find((robot) => robot.id === selectedRobotId) ?? state.robots[0];
  const selectedGridCell = state.cells.find((cell) => cell.label === selectedCell) ?? state.cells[40];

  return (
    <main className="shell">
      <Header state={state} paused={paused} />
      <section className="dashboard">
        <aside className="leftRail">
          <StatsPanel state={state} />
          <Filters
            paused={paused}
            showRoutes={showRoutes}
            showInfertile={showInfertile}
            onPaused={setPaused}
            onRoutes={setShowRoutes}
            onInfertile={setShowInfertile}
          />
          <AirlockPanel state={state} />
        </aside>

        <section className="mapStage">
          <PlayfieldMap
            state={state}
            selectedRobot={selectedRobot}
            selectedCell={selectedCell}
            showRoutes={showRoutes}
            showInfertile={showInfertile}
            onSelectRobot={setSelectedRobotId}
            onSelectCell={setSelectedCell}
          />
        </section>

        <aside className="rightRail">
          <RobotPanel robots={state.robots} selectedRobotId={selectedRobot.id} onSelectRobot={setSelectedRobotId} />
          <DetailPanel cell={selectedGridCell} robot={selectedRobot} />
          <EventLog state={state} />
        </aside>
      </section>
    </main>
  );
}

function Header({ state, paused }: { state: PlayfieldState; paused: boolean }) {
  return (
    <header className="topbar">
      <div>
        <p className="kicker">RAI Terraforming Initiative</p>
        <h1>Playfield Control Panel</h1>
      </div>
      <div className="topStatus">
        <span className={state.emergency ? "statusPill danger" : "statusPill"}>{state.emergency ? "Emergency return" : "Nominal"}</span>
        <span className="statusPill">{paused ? "Paused" : "Live 250 ms"}</span>
        <span className="statusPill">Mock feed</span>
      </div>
    </header>
  );
}

function StatsPanel({ state }: { state: PlayfieldState }) {
  const onField = state.robots.filter((robot) => robot.mode !== "base" && robot.mode !== "stale").length;
  const seeds = state.cells.reduce((sum, cell) => sum + cell.seedCount, 0);
  const fertileRemaining = state.cells.filter((cell) => cell.fertility === "fertile" && cell.seedCount < 2).length;
  const stale = state.robots.filter((robot) => robot.mode === "stale").length;
  return (
    <section className="panel statsGrid">
      <Metric label="robots on field" value={onField} />
      <Metric label="seeds planted" value={seeds} />
      <Metric label="fertile targets" value={fertileRemaining} />
      <Metric label="stale robots" value={stale} tone={stale ? "warning" : "normal"} />
    </section>
  );
}

function Metric({ label, value, tone = "normal" }: { label: string; value: number; tone?: "normal" | "warning" }) {
  return (
    <div className={`metric ${tone}`}>
      <strong>{value}</strong>
      <span>{label}</span>
    </div>
  );
}

function Filters({
  paused,
  showRoutes,
  showInfertile,
  onPaused,
  onRoutes,
  onInfertile,
}: {
  paused: boolean;
  showRoutes: boolean;
  showInfertile: boolean;
  onPaused: (value: boolean) => void;
  onRoutes: (value: boolean) => void;
  onInfertile: (value: boolean) => void;
}) {
  return (
    <section className="panel controlPanel">
      <button className={paused ? "primaryButton paused" : "primaryButton"} onClick={() => onPaused(!paused)}>
        {paused ? "Resume live" : "Pause refresh"}
      </button>
      <label>
        <input type="checkbox" checked={showRoutes} onChange={(event) => onRoutes(event.target.checked)} />
        route overlays
      </label>
      <label>
        <input type="checkbox" checked={showInfertile} onChange={(event) => onInfertile(event.target.checked)} />
        infertile cells
      </label>
    </section>
  );
}

function AirlockPanel({ state }: { state: PlayfieldState }) {
  return (
    <section className="panel">
      <div className="panelTitle">Airlocks</div>
      <div className="airlockList">
        {state.airlocks.map((airlock) => (
          <div className={`airlockRow ${airlock.state}`} key={airlock.id}>
            <div>
              <strong>{airlock.label}</strong>
              <span>{airlock.role}</span>
            </div>
            <div>
              <strong>{airlock.state}</strong>
              <span>{airlock.queue.length ? airlock.queue.join(", ") : "no queue"}</span>
            </div>
          </div>
        ))}
      </div>
    </section>
  );
}

function PlayfieldMap({
  state,
  selectedRobot,
  selectedCell,
  showRoutes,
  showInfertile,
  onSelectRobot,
  onSelectCell,
}: {
  state: PlayfieldState;
  selectedRobot: RobotState;
  selectedCell: string;
  showRoutes: boolean;
  showInfertile: boolean;
  onSelectRobot: (id: string) => void;
  onSelectCell: (label: string) => void;
}) {
  return (
    <div className="arenaWrap">
      <div className="baseArea">
        <div className="baseBox">
          <span>Base</span>
          <strong>Deployment + Parking</strong>
        </div>
        <div className="tunnels">
          {state.airlocks.map((airlock) => (
            <div className={`tunnel ${airlock.state}`} key={airlock.id}>
              <span>{airlock.label}</span>
              <strong>{airlock.state}</strong>
            </div>
          ))}
        </div>
      </div>

      <div className="gridShell">
        {showRoutes && <RouteOverlay robots={state.robots} selectedRobot={selectedRobot} />}
        <div className="grid">
          {state.cells.map((cell) => (
            <button
              className={`cell ${cell.fertility} ${cell.label === selectedCell ? "selected" : ""} ${
                !showInfertile && cell.fertility === "infertile" ? "mutedCell" : ""
              }`}
              key={cell.label}
              onClick={() => onSelectCell(cell.label)}
            >
              <span className="coord">{cell.label}</span>
              <span className="seedBadge">{cell.seedCount}</span>
            </button>
          ))}
        </div>
        {state.robots.map((robot) => (
          <RobotMarker key={robot.id} robot={robot} onSelectRobot={onSelectRobot} />
        ))}
      </div>
    </div>
  );
}

function RouteOverlay({ robots, selectedRobot }: { robots: RobotState[]; selectedRobot: RobotState }) {
  return (
    <svg className="routeLayer" viewBox="0 0 900 900" aria-hidden="true">
      {robots.map((robot) => {
        const points = robot.route.slice(0, 12).map((coordItem) => `${(coordItem.col - 0.5) * 100},${(9.5 - coordItem.row) * 100}`);
        return (
          <polyline
            key={robot.id}
            points={points.join(" ")}
            fill="none"
            stroke={robot.color}
            strokeWidth={robot.id === selectedRobot.id ? 10 : 5}
            strokeLinecap="round"
            strokeLinejoin="round"
            opacity={robot.mode === "stale" ? 0.12 : robot.id === selectedRobot.id ? 0.82 : 0.28}
          />
        );
      })}
    </svg>
  );
}

function RobotMarker({ robot, onSelectRobot }: { robot: RobotState; onSelectRobot: (id: string) => void }) {
  const x = ((robot.coord.col - 0.5) / 9) * 100;
  const y = ((9.5 - robot.coord.row) / 9) * 100;
  return (
    <button
      className={`robotMarker ${robot.mode}`}
      style={{ left: `${x}%`, top: `${y}%`, borderColor: robot.color, color: robot.color }}
      onClick={() => onSelectRobot(robot.id)}
      title={`${robot.id} ${robot.team}`}
    >
      {robot.id}
    </button>
  );
}

function RobotPanel({
  robots,
  selectedRobotId,
  onSelectRobot,
}: {
  robots: RobotState[];
  selectedRobotId: string;
  onSelectRobot: (id: string) => void;
}) {
  return (
    <section className="panel robotPanel">
      <div className="panelTitle">Robots</div>
      <div className="robotList">
        {robots.map((robot) => (
          <button
            key={robot.id}
            className={`robotRow ${robot.id === selectedRobotId ? "active" : ""}`}
            onClick={() => onSelectRobot(robot.id)}
          >
            <span className="robotDot" style={{ background: robot.color }} />
            <span>
              <strong>{robot.id}</strong>
              <small>{robot.team}</small>
            </span>
            <span>
              <strong>{robot.coord.label}</strong>
              <small>{robot.mode}</small>
            </span>
          </button>
        ))}
      </div>
    </section>
  );
}

function DetailPanel({ cell, robot }: { cell: GridCell; robot: RobotState }) {
  return (
    <section className="panel detailGrid">
      <div>
        <div className="panelTitle">Selected Cell</div>
        <strong className="bigValue">{cell.label}</strong>
        <span>{cell.fertility}</span>
        <span>{cell.seedCount} / 2 seeds</span>
        <span>{cell.occupiedBy ? `occupied by ${cell.occupiedBy}` : "unoccupied"}</span>
        <span>{cell.reservedBy ? `reserved by ${cell.reservedBy}` : "no reservation"}</span>
      </div>
      <div>
        <div className="panelTitle">Selected Robot</div>
        <strong className="bigValue">{robot.id}</strong>
        <span>{robot.team}</span>
        <span>
          {robot.coord.label} heading {robot.heading}
        </span>
        <span>target {robot.target.label}</span>
        <span>{robot.seedsRemaining} seeds onboard</span>
      </div>
    </section>
  );
}

function EventLog({ state }: { state: PlayfieldState }) {
  return (
    <section className="panel eventPanel">
      <div className="panelTitle">Event Log</div>
      {state.events.map((event) => (
        <div className={`eventRow ${event.tone}`} key={event.id}>
          <span>{event.time}</span>
          <p>{event.message}</p>
        </div>
      ))}
    </section>
  );
}

createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>,
);
