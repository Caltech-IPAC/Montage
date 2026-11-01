# Montage: Astronomical Image Mosaics, Examination, and Visualization

[![License: BSD-3-Clause](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](LICENSE)
[![PyPI](https://img.shields.io/pypi/v/MontagePy)](https://pypi.org/project/MontagePy/)
[![ascl:1010.036](https://img.shields.io/badge/ascl-1010.036-blue.svg)](https://ascl.net/1010.036)

Montage is an open-source toolkit from Caltech-IPAC for assembling Flexible Image Transport System (FITS) images into custom mosaics. You choose the coordinates, projection, pixel scale, and rotation, and whether to match backgrounds. It ships as C command-line programs, a C library, and the MontagePy Python package.

**Releases:** the latest GitHub release is [v6.0](https://github.com/Caltech-IPAC/Montage/releases/tag/v6.0) (November 2018); the later `v6.1` tag (August 2021) has no published release. `main` carries all later work, including the 7.0 additions recorded in [ChangeHistory](ChangeHistory) (HiPS tools, `mAddMem`) and recent fixes. MontagePy is versioned separately.

## What Montage does

| Category | Key modules | Purpose |
| --- | --- | --- |
| Reprojection | `mProject`, `mProjectPP`, `mProjectQL`, `mProjExec` | Put FITS images on a common pixel grid with general, tangent-plane, or quick-look algorithms; process image lists in bulk. |
| Background matching | `mOverlaps`, `mDiff`, `mFitplane`, `mBgModel`, `mBackground`; `mDiffExec`, `mFitExec`, `mBgExec` | Find overlaps, fit image differences, derive background corrections, and apply them individually or in batches. |
| Coaddition | `mAdd`, `mAddMem` | Combine reprojected images into a mosaic, with an in-memory option for deep stacks. |
| Metadata and headers | `mImgtbl`, `mMakeHdr`, `mGetHdr` | Inventory FITS image geometry, define an output region, and extract editable headers. |
| Visualization | `mViewer` | Render grayscale or three-color FITS images as PNG/JPEG, with coordinate grids and catalog/image overlays. |
| Archive access | `mArchiveList`, `mArchiveGet`, `mArchiveExec` | Discover archive images and download them by URL, individually or from a table. |
| HiPS | `mHPXMosaicScripts`, `mHPXShrinkScripts`, `mHiPSTiles`, `mHiPSPNGScripts` | Generate HPX mosaic workflows, lower-resolution plates, and hierarchical FITS/PNG survey tiles. |
| Data cubes | `mProjectCube`, `mAddCube`, `mSubCube`, `mShrinkCube` | Reproject and coadd FITS cubes, extract sub-cubes, and reduce their resolution. |
| Image utilities | `mSubimage`, `mShrink`, `mFixNaN`, `mRotate` | Make cutouts, resize images, handle invalid pixels, and rotate images. |

`mProject` and `mProjectPP` conserve flux; `mProjectQL` favors speed for quick-look visualization. The bundled WCS library supports HEALPix and TOAST projections.

The HiPS workflow generators emit scripts for serial or SLURM processing; some can also emit AWS CLI commands for S3 transfers. See [HiPS/Notes.txt](HiPS/Notes.txt) for the plate-based workflow and its site-specific setup assumptions.

## Installation

### Python

```sh
python -m pip install MontagePy
```

PyPI 2.3.0 provides wheels for CPython 3.7–3.13 on Linux x86-64 and 3.8–3.13 on macOS 11+. There are no source distributions, Windows wheels, or Linux arm64 wheels.

> [!WARNING]
> **Apple Silicon:** PyPI 2.3.0's macOS wheels contain Intel binaries, even those labeled `arm64`. Use a native `arm64` wheel from [`built_wheels_v2.3.1/`](python/MontagePy/built_wheels_v2.3.1/) matching your Python version (3.9–3.13), for example:
>
> ```sh
> python -m pip install python/MontagePy/built_wheels_v2.3.1/montagepy-2.3.1-cp39-cp39-macosx_11_0_arm64.whl
> python -c 'from MontagePy.main import mImgtbl; print("ok")'
> ```

The scripts in [`python/MontagePy/`](python/MontagePy/) generate Cython wrappers from the `MontageLib/*/*.json` API descriptions and build wheels with `cibuildwheel` (`macos_wheels/`, `linux_wheels/`). Building the C toolkit does not build or install MontagePy.

### Building from source

You need a C compiler available as `cc`/`gcc`, GNU Make 3.81+, `ar`, `ranlib`, and Bash. Third-party libraries are bundled. Build and quick start verified on Apple Silicon with Apple Command Line Tools.

```sh
./configure --prefix="$HOME/montage"   # default prefix: /usr/local
make
mkdir -p "$HOME/montage/bin"           # make install does not create it
make install
export PATH="$HOME/montage/bin:$PATH"
```

- `./configure` only writes the prefix into `config.make`, a tracked file. It does not check dependencies.
- `make` builds the bundled libraries, the command-line tools (into `bin/` at the top of the tree), and the C library (`MontageLib/libmontage.a` and `libmontage.so`). Compiler warnings are expected.
- `make install` copies `bin/*` to `<prefix>/bin` and nothing else: no headers, libraries, or data. Adding the tree's own `bin/` to `PATH` works too.
- **Keep the source tree after installing.** On macOS, `mViewer` loads the bundled FreeType library from the build tree by absolute path. On every platform, it reads `FreeSans.ttf` for text overlays such as grid labels from the tree's `data/fonts/` unless `MONTAGE_FONT_DIR` names another directory; without it, those renders fail with `FreeType: Could not find or load font file.`

Bundled third-party sources in `lib/src/`:

| Library | Version |
| --- | --- |
| CFITSIO | 3.49 (with zlib 1.2.5; built without curl) |
| WCSTools WCS subroutines | 3.9.0, Montage-modified (`wcssubs3.9.0_montage`) |
| bzip2 | 1.0.6 |
| IJG JPEG | 8b |
| FreeType | 2.13.3 |
| LodePNG | 20140823 |
| Plane-to-plane transforms | `two_plane_v1.1` |

The manylinux wheel scripts (`python/MontagePy/linux_wheels/`) also build the C sources on Linux x86-64.

**Windows:** [Windows/Notes.txt](Windows/Notes.txt) describes a Docker/MinGW cross-build of native 64-bit executables, with some Unix-only modules left out. It is out of date: `Windows/Makefiles/Makefile.lib` still references `cfitsio-3.25` and `freetype-2.9.1`, which are no longer in `lib/src/`.

## Quick start

Start with a directory `raw/` of overlapping two-dimensional FITS images that have valid WCS headers and compatible units. With the Montage tools on `PATH`, run this from the directory that contains `raw/`:

```sh
mkdir -p projected
mImgtbl raw images.tbl
mMakeHdr images.tbl region.hdr
mProjExec -p raw images.tbl region.hdr projected stats.tbl
mImgtbl projected projected.tbl
mAdd -p projected projected.tbl region.hdr mosaic.fits
mViewer -gray mosaic.fits min max gaussian-log -out mosaic.png
```

This builds a common header, reprojects the inputs, coadds them into `mosaic.fits`, and renders `mosaic.png`. Keep the `_area.fits` coverage images written during reprojection: `mAdd` uses them for weighting by default.

To match backgrounds, replace the last two commands with:

```sh
mkdir -p diffs corrected
mOverlaps projected.tbl diffs.tbl
mDiffExec -p projected diffs.tbl region.hdr diffs
mFitExec diffs.tbl fits.tbl diffs
mBgModel projected.tbl fits.tbl corrections.tbl
mBgExec -p projected projected.tbl corrections.tbl corrected
mImgtbl corrected corrected.tbl
mAdd -p corrected corrected.tbl region.hdr mosaic.fits
mViewer -gray mosaic.fits min max gaussian-log -out mosaic.png
```

The same uncorrected mosaic from Python:

```python
from pathlib import Path
from MontagePy.main import mImgtbl, mMakeHdr, mProjExec, mAdd

Path("projected").mkdir(exist_ok=True)
print(mImgtbl("raw", "images.tbl"))
print(mMakeHdr("images.tbl", "region.hdr"))
print(mProjExec("raw", "images.tbl", "region.hdr",
                projdir="projected", stats="stats.tbl"))
print(mImgtbl("projected", "projected.tbl"))
print(mAdd("projected", "projected.tbl", "region.hdr", "mosaic.fits",
           shrink=True, haveAreas=True))
```

Each call returns a status dictionary. Check `status` (and failure counts such as `failed`) before moving on. The Python `mAdd` defaults to `haveAreas=False`, unlike the command-line tool, so pass it explicitly.

## Repository layout

| Directory | Contents |
| --- | --- |
| `Montage/` | Supplementary command-line programs (`mConvert`, `mTileImage`, …) and optional MPI targets. |
| `MontageLib/` | The core modules in library form, with command-line drivers, JSON API descriptions, and library build rules. |
| `util/` | Additional image, catalog, search, and workflow utilities. |
| `HiPS/` | HPX/HiPS processing tools, workflow-script generators, and workflow notes. |
| `ancillary/` | `HPXcvt`, a HEALPix-to-2D-FITS converter. |
| `grid/` | Pegasus workflow tools and Globus execution/notification utilities. |
| `web/` | TAP query tools (`mTAP`) and web `mViewer` sources (not built by default). |
| `react/` | React component for displaying Montage images in Plotly Dash (not built by default). |
| `man/` | HTML module documentation, tutorials, and viewer guides. |
| `data/` | Fonts used by `mViewer`. |
| `python/` | MontagePy wrapper generation, packaging, and wheel builds. |
| `lib/` | Bundled third-party sources and their build rules. |
| `Windows/` | Windows cross-build scripts, makefiles, and supporting libraries. |
| `build/` | Release-packaging scripts and maintainer notes. |

## Documentation & support

| Resource | Link |
| --- | --- |
| User documentation | [Documentation index](http://montage.ipac.caltech.edu/docs/index.html) |
| Command reference | [Module usage and arguments](http://montage.ipac.caltech.edu/docs/index2.html) |
| Mosaic tutorial | [Build a 2MASS mosaic of M101](http://montage.ipac.caltech.edu/docs/m101tutorial.html) |
| Python guide | [MontagePy user guide](http://montage.ipac.caltech.edu/docs/montagePy-UG.html) |
| Bug reports and support | [GitHub issues](https://github.com/Caltech-IPAC/Montage/issues) |

The project contact address is montage@ipac.caltech.edu.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for how the code is organized and where contributions are welcome. Recent pull requests target `main`, the default branch. [Jenkins](Jenkinsfile) runs `make`; there is no automated test stage yet.

## Citing

If you use Montage in research, cite the software record [ascl:1010.036](https://ascl.net/1010.036). For the toolkit's design, see Jacob et al., *Montage: a grid portal and software toolkit for science-grade astronomical image mosaicking*, Int. J. Computational Science and Engineering (2009); [arXiv:1005.4454](https://arxiv.org/abs/1005.4454).

Suggested acknowledgment:

> This research made use of Montage, which received funding from the National Science Foundation under Grant Number ACI-1440620 and from the National Aeronautics and Space Administration's Earth Science Technology Office, Computation Technologies Project, under Cooperative Agreement Number NCC5-626 between NASA and the California Institute of Technology.

## License

Montage is distributed under the [BSD 3-Clause license](LICENSE). Bundled third-party components under `lib/src/` keep their own license terms; for example, [FreeType](lib/src/freetype-2.13.3/LICENSE.TXT) and [bzip2](lib/src/bzip2-1.0.6/LICENSE).

## Funding and credits

Montage was funded from 2002 to 2005 by NASA's Earth Science Technology Office, Computation Technologies Project, under Cooperative Agreement NCC5-626 between NASA and Caltech. It was later supported by NSF award ACI-1440620, *The Next Generation of the Montage Mosaic Engine* (2014–2017).

`mProjectPP` is an adaptation of the MOPEX algorithm developed at the Spitzer Science Center.
