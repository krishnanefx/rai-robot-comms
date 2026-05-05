import { useEffect, useMemo, useState } from "react";
import { createPlayfieldState } from "./mockPlayfield";
import type { PlayfieldState } from "./types";

export const refreshMs = 250;

export function usePlayfieldState(paused: boolean): PlayfieldState {
  const [tick, setTick] = useState(0);

  useEffect(() => {
    if (paused) return;
    const id = window.setInterval(() => setTick((value) => value + 1), refreshMs);
    return () => window.clearInterval(id);
  }, [paused]);

  return useMemo(() => createPlayfieldState(tick), [tick]);
}

