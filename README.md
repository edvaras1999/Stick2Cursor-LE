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

First experimental source. Cursor motion uses relative mouse input rather than SetCursorPos so Skyrim's UI receives the motion instead of moving the desktop pointer. Verify Skyrim and KHAOS cursor behaviour in game before distributing a test build.

## Project layout

- `src/`: SKSE LE plugin source
- `papyrus/src/`: SkyUI MCM script source
- `dist/`: installable configuration and scripts

