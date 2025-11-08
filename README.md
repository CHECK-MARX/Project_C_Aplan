# Car Cockpit + HVAC Simulator

Small Win32 desktop simulator that renders a car cockpit with gauges, indicators, and an HVAC control cluster. All logic is written in portable C so the simulation core can be tested headlessly.

## Build & Run

1. Open a regular command prompt and change into this folder.
2. Execute `build_msvc.bat`. The script locates the latest Visual Studio 2019 toolset via `vswhere`, sets up the environment, and runs:
   ```
   cl /nologo /utf-8 /TC /W4 /WX- /permissive- /Zc:wchar_t /EHsc- ^
      /DUNICODE /D_UNICODE ^
      src\main.c src\sim.c src\ui.c src\input.c ^
      user32.lib gdi32.lib
   ```
3. Launch the produced `main.exe`. The window is resizable; repainting is driven by a 60 Hz timer.

## Key Bindings

| Key            | Action |
|----------------|--------|
| ← / →          | Toggle left / right turn signals |
| H              | Toggle hazard flashers |
| L              | Toggle headlight indicator |
| ↑ / ↓          | Increase / decrease throttle (0–100 %) |
| Space          | Hold brake (0–100 %) |
| A              | Toggle A/C |
| F              | Cycle fan speed 0–7 (manual mode) |
| R              | Toggle recirculation |
| D              | Toggle defrost |
| O              | Toggle HVAC AUTO mode |
| [+] / [-]      | Adjust temperature setpoint in 0.5 °C steps |
| M              | Cycle airflow mode (face → bi-level → foot) |

AUTO mode enforces fan level, airflow, defrost, and AC engagement based on the cabin vs. setpoint delta. Manual changes to fan or airflow automatically exit AUTO.

## QAC_TRAINING knob

`src/sim.c` contains `#define QAC_TRAINING 0`. Flip it to `1` to introduce a deliberately uninitialized local variable path inside `sim_step()`. This is useful for demonstrating static analysis tooling; leave it at `0` for normal builds.

## Notes

- Simulation tick runs at 60 Hz via a timer and high-resolution clock, and the HVAC thermal model follows the provided first-order dynamics.
- The UI renderer uses off-screen bitmaps for flicker-free GDI painting and keeps all GDI objects owned by `UiState`.
- Speed and RPM gauges now show colored safety bands (green/yellow/red) plus numeric tick labels for faster readability.
# Project_C_Aplan
