How MontagePy wheels are built
==============================

A MontagePy wheel is the Montage C library compiled into one Python
extension, plus a thin layer of generated Cython wrappers and a few pure
Python helpers. All bundled native libraries come from `lib/src/` in the
checkout. Every wheel is built from the checked out commit, so the wheel
and the source it came from are always the same revision.

This file describes the pipeline. For installing a released wheel see
README.md in this directory. For the release procedure see the
Maintainer release recipe in the same file.


What goes into a wheel
----------------------

| Part | Source | Produced by |
|---|---|---|
| `MontagePy._wrappers` extension | `MontageLib/*/` C sources and their JSON signatures | Montage `make`, then `parse.py` and Cython |
| `MontagePy.main` extension | `templates/template_main.pyx` | `parse.py` and Cython |
| `MontagePy/_wrappers.c`, `MontagePy/main.c` | the Cython output for the two extensions | Cython, kept in the wheel alongside the compiled modules |
| `MontagePy/__init__.py` | `__init__.py` | copied by `stage_package.sh` |
| `MontagePy.archive` | `__archive__.py`, `mArchiveList.py`, `mArchiveDownload.py` | copied by `stage_package.sh` |
| `MontagePy/FreeSans.ttf` | `data/fonts/FreeSans.ttf` | copied by `stage_package.sh` |
| bundled native dependencies, including CFITSIO, zlib, WCS and JPEG | `lib/src/` | Montage `make`, collected by `MontageLib`'s `pythonlibs` target and linked in as objects |

The native dependencies are linked into the `MontagePy._wrappers`
extension from the object files that the top level Montage build leaves
in `python/MontagePy/lib/`. A wheel does not need a separately installed
CFITSIO or any of the other bundled native libraries. The only Python
dependency is `requests`, used by the archive helpers.


The pipeline
------------

1. Build Montage. The top level `make` compiles every library and
   module and leaves the objects the wheel needs in
   `python/MontagePy/lib/`. Under cibuildwheel `prepare_checkout.sh`
   runs this step with `USE_SYSTEM_CFITSIO=0`, so the bundled CFITSIO
   is always what ends up in a wheel, and passes every bundled configure
   script a build type from the newer CFITSIO `config.guess` because the
   JPEG copy cannot detect some hosts.

2. Stage the package. `stage_package.sh` rebuilds `src/MontagePy` from
   scratch, copies the pure Python files and the font in, runs
   `parse.py` to turn the JSON signature files in `MontageLib` into
   `_wrappers.pyx`, `wrappers.pxd` and `main.pyx`, and runs
   `cleanup.py` over the generated wrappers. It needs `jinja2`.

3. Build the wheel. `setup.py` cythonizes the two `.pyx` files and
   links `_wrappers` against every object in `python/MontagePy/lib/`.
   It forces `ARCHFLAGS` to the host machine type so a macOS build
   never produces a universal binary by accident.

4. Test the wheel. cibuildwheel installs the fresh wheel into a clean
   environment and runs `tests/montagepy_smoke.py` against it. On
   macOS, `check_macos_wheel.py` then opens each wheel and runs `lipo`
   on every extension to confirm it holds exactly the architecture the
   platform tag claims.

5. Publish. Each runner uploads its wheels as a workflow artifact. On a
   release tag the release job downloads all of them and attaches them
   to a GitHub Release named after the tag. The release is not marked
   latest, so the Montage source releases keep that spot.

Locally the same chain is `make` at the top level, then
`sh python/MontagePy/make_local.sh`, which runs `stage_package.sh` and
`python -m build --wheel` and leaves the wheel in `dist/`. It needs
`build` and `jinja2` installed. Cython and setuptools come from the
isolated build environment that `build` creates.


Where it runs
-------------

`.github/workflows/wheels.yml` drives cibuildwheel on three runners.

| Runner | Wheels | Notes |
|---|---|---|
| `ubuntu-24.04` | `manylinux2014_x86_64` | built inside the manylinux container, then `auditwheel repair` |
| `macos-15` | `macosx_11_0_arm64` | no repair step, nothing to bundle |
| `macos-15-intel` | `macosx_11_0_x86_64` | no repair step, nothing to bundle |

Each runner builds CPython 3.9 through 3.15, so a full run produces 21
wheels. musllinux is skipped. `MACOSX_DEPLOYMENT_TARGET` is 11.0 on
both macOS runners, which is also what the wheel tag says.

The workflow runs on three events.

- A push of a tag matching `MontagePy-v*` builds the full matrix and
  publishes a release. The `version` job first checks that the tag
  matches the package version.
- A manual dispatch builds the full matrix and keeps the wheels as
  artifacts without publishing.
- A pull request that touches this directory, the smoke test, the
  architecture checker test, `lib/src/Makefile`, or the workflow
  builds CPython 3.12 only on each runner. That keeps every platform
  leg exercised before merge while building one wheel per runner
  instead of the full set. `lib/src/Makefile` is listed because a
  CFITSIO upgrade edits it and `prepare_checkout.sh` names the CFITSIO
  directory. `ci.yml` separately builds Montage and Linux wheels on
  every pull request.

All cibuildwheel settings live in `[tool.cibuildwheel]` in
`pyproject.toml`. The workflow only adds `CIBW_ARCHS` per runner and,
for pull requests, `CIBW_BUILD`. Actions are pinned to commit SHAs.


Versions
--------

The package version is the `version` field in `pyproject.toml`.

`check_version.py` reads it and fails if it is missing or is not a
nonempty string. With `--tag MontagePy-vX.Y.Z` it also fails if the
tag does not match, if the version is not three dotted integers, or if
it is not newer than 2.3.1. The historical 2.3.1 wheels were
distributed from `built_wheels_v2.3.1/` in this repository, and pip
needs a newer version to upgrade those installations.


Tests that guard the pipeline
-----------------------------

All but the smoke test run from `make test` at the top level and are
listed in `tests/run_tests.sh`. `make test` skips the architecture
checker test off macOS, so the wheel workflow also runs it on both
macOS runners. The smoke test runs against an installed wheel, by
cibuildwheel in the wheel workflow and by `ci.yml`.

| Test | Guards |
|---|---|
| `tests/check_version.sh` | the readable version, the tag rules, the format guard and the floor |
| `tests/check_macos_wheel.sh` | the accept and reject cases of `check_macos_wheel.py`, using small Mach-O objects built on the fly, macOS only |
| `tests/wheel_staging_guard.sh` | `stage_package.sh` and `make_local.sh` only ever delete inside this directory, whatever the caller's working directory, `CDPATH`, a symlinked script path, or a script name that `sh` found through `PATH` |
| `tests/wheel_objects.sh` | the staged native objects link together without missing internal symbols, catching basename collisions during staging |
| `tests/montagepy_smoke.py` | the installed wheel imports, the bundled CFITSIO works, mMakeImg calls are isolated, mExamine reports dimensions correctly, known image values match, positional arguments reach the library in signature order, and a synthetic image and cube pipeline gives identical results under junk filled allocations |


Legacy directories
------------------

`linux_wheels/` and `macos_wheels/` hold the scripts used for the
2.3.1 wheels. They clone Montage from GitHub rather than building the
checkout. cibuildwheel takes its settings from the `pyproject.toml` in
each directory, but the package is built with the `pyproject.toml`
copied from that clone. They are not used by the workflow and are due
to be consolidated.
`built_wheels_v2.3.1/` keeps the 2.3.1 binaries for reference only.
