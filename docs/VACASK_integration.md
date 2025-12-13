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

## 7. Mapping the diode example to Qucs-S
The VACASK diode regression shown below exercises nested sweeps, OSDI model loading, and control/post-process sections:

```
ground 0

load "diode.osdi"
load "resistor.osdi"

model vsource vsource
model resistor resistor
model d diode is=1e-12 n=2 rs=0.1 cjo=100p vj=1 m=0.5

v1 (1 0) vsource dc=0
r1 (1 2) resistor r=1
d1 (2 0) d

control
  abort always
  save default p(d1,gd) p(d1,cd)
  sweep is model="d" parameter="is" values=[1e-12, 1e-10, 1e-8, 1e-6] continuation=1
  sweep v1 instance="v1" parameter="dc" from=-50 to=10 mode="lin" points=200 continuation=1
    analysis op1 op
  postprocess(PYTHON, "runme.py")
endc
```

To support this workflow inside Qucs-S:

1. **OSDI model loading** – Allow `load "*.osdi"` lines to pass through the VACASK netlist unchanged. In the kernel’s `createNetlist`, treat these like Ngspice `.include` statements so files referenced from the schematic are emitted at the top of the deck.
2. **Node parentheses** – Reuse existing node ordering but wrap every node name with parentheses when instantiating devices (`v1 (1 0) vsource`). Implement this in the VACASK kernel’s element writers so the rest of the SPICE syntax remains identical.
3. **Model/sweep parameters** – Map Qucs device parameters to VACASK model cards (e.g., diode `is`, `n`, `rs`). When the schematic contains Parameter Sweep components, emit nested `sweep` blocks in the control section matching the example (outer `is` sweep, inner voltage sweep) and flag `continuation=1` to reuse operating points between sweeps.
4. **Control block and analyses** – Emit a `control`/`endc` pair for VACASK rather than Ngspice’s `.control`. Insert `abort always` and `save` statements up front, then translate Qucs analyses into `analysis` commands (e.g., DC op as `analysis op1 op`). If multiple analyses are queued, follow the same ordering used for Ngspice in `ExternSimDialog::slotStart`.
5. **Post-processing hooks** – Preserve `postprocess(PYTHON, "runme.py")` lines in the generated netlist and copy referenced scripts into the temporary simulation directory. After VACASK finishes, allow the kernel to leave the `.raw` output on disk so the Python helper can read it before Qucs cleans up.
6. **CLI invocation** – Launch VACASK as `vacask <netlist>.sim` from `slotSimulate`, mirroring the existing process management (stdin closed, stdout/stderr captured). Keep the produced `.raw` file name predictable (`op1.raw` in the example) so `convertToQucsData` can import it into a dataset.
7. **Dataset import** – If VACASK writes standard SPICE RAW, reuse the Ngspice parser path. Otherwise, add a small converter that reads the `.raw` file before the Python post-processor runs so both Qucs datasets and the regression script see the same file.
