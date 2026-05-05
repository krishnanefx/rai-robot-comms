# Contributing

This repo is for shared course tooling. Keep changes small, documented, and easy
for classmates to use in Arduino IDE.

## Before Opening A PR

Run:

```bash
bash scripts/check.sh
```

For dashboard-only changes:

```bash
cd dashboard
npm run build
```

## Code Guidelines

- Keep Arduino APIs stable and small.
- Do not put professor-server endpoint guesses into student sketches.
- Put all real server integration behind `ProfessorServerAdapter`.
- Keep examples runnable without private WiFi credentials where possible.
- Update `AGENTS.md` and `docs/arduino-integration-guide.md` when changing integration steps.

## Secrets

Never commit `arduino_secrets.h`, WiFi passwords, tokens, or server keys.
