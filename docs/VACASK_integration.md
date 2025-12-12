# Integrating the VACASK simulator

The VACASK simulator is SPICE-compatible with minor netlisting differences (e.g., ports must be enclosed in parentheses). Follow this checklist to add it alongside Ngspice and Xyce.

## 1. Register the simulator
- Extend `spicecompat::Simulator` and `getDefaultSimulatorName` with a `Vacask` entry.
- Add default path/parameters to `QucsSettings` and the `_settings` map (mirroring `Ngspice` and `Xyce`).
- Wire the new enum value anywhere a simulator is listed (combo boxes, defaults, etc.).

## 2. Implement the kernel
- Create `Vacask` deriving from `AbstractSpiceKernel` under `qucs/extsimkernels/`.
- Override `createNetlist` to wrap port names in parentheses while reusing the existing Ngspice-style element mapping.
- Implement `slotSimulate` and `slotProcessOutput` (or reuse base implementations if compatible) and expose `setSimulatorCmd/Parameters` like other kernels.
- Add the new sources to `qucs/extsimkernels/CMakeLists.txt`.

## 3. Netlisting tweaks
- When writing subcircuit definitions and instance connections, emit nodes as `(node)` to satisfy VACASK’s parser.
- Keep other SPICE syntax identical to Ngspice unless VACASK requires additional options; reuse Ngspice model card handling.
- If `.options` lines differ, add a small helper in the kernel to inject them before simulation start.

## 4. UI wiring
- Update `ExternSimDialog` switch blocks to instantiate the `Vacask` kernel, route its signals, and handle start/stop/save-netlist actions.
- Extend `SimSettingsDialog` with path/parameter fields for VACASK and store them in `slotApply`.
- Add the new option anywhere the simulator selector is populated (`fillSimulatorsComboBox`, default preferences, etc.).

## 5. Conversion and parsing
- If VACASK output is SPICE-standard, reuse `AbstractSpiceKernel::convertToQucsData`. Otherwise, adjust `slotProcessOutput` to normalize VACASK traces into the existing dataset conversion flow.
- Add warning/error pattern entries if VACASK emits distinctive diagnostics.

## 6. Testing
- Build to ensure the new kernel is compiled and moc’d.
- Run simulations for representative circuits: DC operating point, AC sweep, transient, and a subcircuit case to verify parenthesized ports are accepted.
- Verify netlist saving, default simulator selection, and settings persistence across restarts.
