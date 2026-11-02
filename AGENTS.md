# AGENTS.md

## Project Overview

- Montage is a C toolkit for FITS image reprojection, background matching,
  mosaicking, visualization, and HiPS generation. It ships CLI tools, C libraries,
  and the MontagePy Python extension.

## Where To Start

- Read [README.md](README.md) for scope and [CONTRIBUTING.md](CONTRIBUTING.md)
  for contribution guidance; most contributions target the `develop` branch.
- Start with [man/docs/index.html](man/docs/index.html) for user documentation,
  [man/docs/index2.html](man/docs/index2.html) for command reference, and
  [man/docs/design.html](man/docs/design.html) for the mosaic processing stages.
- Use current Makefiles, C headers, and build scripts to resolve disagreements
  with older HTML documentation or packaging notes.

## Repository Map

- `MontageLib/`: module directories containing library implementations, CLI
  drivers, headers, and JSON interface descriptions; `MontageLib/montage.h`
  declares the public C functions and return structures.
- `Montage/`: standalone implementations and shared helpers; overlaps with
  `MontageLib/`, but also supplies commands absent from the library build.
- `lib/src/`: bundled dependencies and support libraries; its `Makefile`
  selects the versions actually built.
- `util/`: additional command-line utilities; `grid/`: Pegasus and Globus tools.
- `HiPS/`: headers, workflow scripts, tiles, PNG generation, and gap handling;
  `ancillary/`: the HPX conversion utility.
- `python/MontagePy/`: Python packaging, wrapper generation, and platform wheel builds.
- `web/`: mViewer and mTAP web services; `react/`: mViewer React component for Dash.
- `Windows/`: cross-compilation scripts, alternate Makefiles, and dependencies.
- `man/`: HTML manuals and tutorials; `data/fonts/`: bundled visualization fonts.
- `build/`: release packaging notes/scripts; `ChangeHistory`: release history.
- `Jenkinsfile` and `tests/testsuite.xml`: CI definition and checked-in test report.

## Build And Run

Run commands from the repository root unless a different directory is stated.

| Command | Purpose / prerequisite |
| --- | --- |
| `make` | Full build used by Jenkins; creates local executables, libraries, and Python extension object inputs. |
| `make -C MontageLib` | Rebuild library-backed tools, C libraries, and Python object inputs after the full build has supplied dependencies. |
| `make -C MontageLib/Project` | Example targeted module rebuild after dependencies and shared helper objects exist. |
| `make clean` | Remove build outputs through the root Makefile's recursive clean targets. |
| `./configure --prefix=PREFIX` | Optional: set the installation prefix in `config.make`; substitute the desired directory. |
| `make install` | Copy built executables into the configured prefix's existing `bin` directory. |
| `./make_local.sh` in `python/MontagePy/` | Build a local wheel after `make`; requires Python with Jinja2 and `build`, plus the backend requirements in `pyproject.toml`. |

- Run tools from the generated root `bin` directory or add that directory to
  the executable search path; see the command reference for required FITS inputs.
- Use serial `make`: `MontageLib/Makefile` lists `pgm`, `lib`, and `pythonlibs`
  without dependency edges even though the latter targets consume compiled objects.
- The local wheel script replaces its generated source, build, and distribution
  directories. Platform wheel scripts clone upstream; inspect them before using
  them to validate changes in the current checkout.

## Implementation Boundaries

- In `MontageLib/Project/`, `mProject.c` parses CLI arguments and calls the
  implementation in `montageProject.c`; `mProject.h` holds module declarations.
  Use this layout to navigate analogous modules.
- The root build installs `Montage/` first and `MontageLib/` later. Library-backed
  executables replace overlapping names in the output directory. Trace the build
  before choosing which implementation to edit or which executable to test.
- When changing a library interface, check `MontageLib/montage.h`, the module's
  CLI driver, and its JSON description (for example,
  `MontageLib/Project/mProject.json`) together.
- `python/MontagePy/parse.py` generates Cython wrappers from module JSON and
  `python/MontagePy/templates/`; edit these inputs for wrapper changes.
  `MontageLib/Makefile` collects the object files used by the extension.
- `lib/src/Makefile` builds the customized WCS sources in
  `lib/src/wcssubs3.9.0_montage/`; other WCS source trees are also present.
- The root build does not build the web services, React component, Windows
  distribution, or Python wheels; those have separate workflows.

## Validation

- `Jenkinsfile` runs `make`; its test stage is a placeholder and publishes the
  checked-in XML report. A successful CI build does not establish regression coverage.
- The root Makefile defines no test, lint, or format target. For behavior changes,
  exercise the affected tool/API with representative FITS inputs and record the
  commands, data, and results; see `CONTRIBUTING.md` for testing limitations.
- `MontageLib/test/` contains an example, but its Makefile references a missing
  `mTest.c`; it is not a working general test entrypoint.

## Deeper References

- Algorithms and numerical behavior: [man/docs/algorithms.html](man/docs/algorithms.html).
- HiPS processing workflow: [HiPS/Notes.txt](HiPS/Notes.txt).
- Viewer documentation: [man/mViewer/index.html](man/mViewer/index.html).
- React integration: [react/README.md](react/README.md).
