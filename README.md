# Stick2Cursor LE

Gamepad cursor control for Skyrim Legendary Edition 1.9.32 and SKSE 1.7.3.

## Planned behaviour

- SkyUI MCM selects Toggle or Hold activation.
- A configurable two-button gamepad chord activates cursor mode.
- Cursor speed is configurable.
- Cursor source is configurable: right stick or D-pad.
- The chosen gamepad input is suppressed from its normal action while cursor mode is active, then restored on exit.
- Settings are saved by the SKSE plugin in `Data/SKSE/Plugins/Stick2CursorLE.ini`.

## Development status

Project scaffold only. Input suppression and cursor injection must be verified against the LE 1.9.32 runtime before a test build is distributed. No runtime behaviour is claimed yet.

## Project layout

- `src/`: SKSE LE plugin source
- `papyrus/src/`: SkyUI MCM script source
- `dist/`: installable configuration and scripts

