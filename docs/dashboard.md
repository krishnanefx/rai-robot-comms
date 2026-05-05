# Dashboard

The dashboard is a React/Vite control panel for students watching the playfield.

Current behavior:

- 9x9 playfield grid
- base and airlock status
- 14 mock robots
- route overlays
- fertility and seed-count badges
- selected cell/robot detail
- important event log
- 250 ms mock refresh loop

## Run Locally

```bash
cd dashboard
npm install
npm run dev
```

Open the local URL printed by Vite.

## Build

```bash
cd dashboard
npm run build
```

## Smoke Test

Start the dev server in one terminal:

```bash
cd dashboard
npm run dev
```

Then run:

```bash
cd dashboard
npm run smoke
```

The smoke test verifies the expected title, 81 cells, 14 robot markers, 14 route
overlays, and no horizontal overflow at a projector-sized viewport.

## Switching To The Professor Server

Keep the UI components independent of transport details. Add the polling adapter
inside `dashboard/src/dataSource.ts` and make it return the same shape as
`PlayfieldState` in `dashboard/src/types.ts`.

Use `.env.example` as the template for future server configuration.
