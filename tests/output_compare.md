# Montage output comparison

`tests/output_compare.sh` runs the same offline pipeline with two Montage builds on one machine. It compares every generated input, FITS image, area image, table, PNG, stdout log and stderr log byte for byte with `cmp`. It also reports files present in only one run.

This checks the exercised pipeline, not every input or Montage program. Matching output is not an independent check of scientific correctness. Two builds can produce the same incorrect result.

## Run locally

Build two source trees with the same compiler and build settings, then pass their `bin` directories to the script from the candidate tree:

```sh
make -C BASE
make -C HEAD
sh tests/output_compare.sh BASE/bin HEAD/bin
sh tests/output_compare.sh BASE/bin HEAD/bin comparison-work
```

The script needs POSIX `sh` and ordinary command-line utilities, including `cmp`, `awk`, `sed`, `dd`, `diff`, `find`, `sort`, `gzip`, `gunzip` and `mktemp`. No image data is downloaded. Python is not needed to run the pipeline or decide the result.

The optional work directory receives `base`, `candidate` and `compare` directories. Those names must not already exist. Other files are left alone, and a supplied directory is kept on success or failure. Without this argument, a temporary directory is removed on exit. Set `KEEP=1` to keep it and print its path:

```sh
KEEP=1 sh tests/output_compare.sh BASE/bin HEAD/bin
```

Exit codes are:

- `0`: all files are identical or their differences were explicitly expected
- `1`: at least one unexpected file difference
- `2`: setup or a pipeline step failed, such as a missing executable, a nonzero exit, a missing `stat="OK"`, or a reported image failure

Every Montage invocation must exit successfully and report `stat="OK"`. The image counts and failure counts that the programs report are checked too. Each invocation has separate `logs/STEP.stdout` and `logs/STEP.stderr` files. The report prints `same PATH`, `DIFF PATH`, or `DIFF (expected) PATH`, then a summary count.

## Expected differences

`EXPECT_DIFF` contains optional space-separated shell filename patterns matched against the whole relative output path. A `*` can match `/`. Quote the value so the calling shell does not expand it:

```sh
EXPECT_DIFF='products/int*.fits logs/tile-*.stdout' sh tests/output_compare.sh BASE/bin HEAD/bin
```

Expected differences remain visible and get FITS diagnostics. A pattern does not require a difference to occur or excuse a pipeline failure. Leave this variable unset when checking for identical output.

## Coverage and skips

The script generates three overlapping 64 by 64 TAN images with different centers, rotations, backgrounds and noise levels. A table supplies three point sources with Gaussian or zero-width profiles. The shared 112 by 112 mosaic template is rotated by 17 degrees. A gzip copy exercises compressed input.

The pipeline covers:

- `mMakeImg`, `mImgtbl` and `mTblSort` for generated images, metadata and numeric sorting in both directions
- `mProject`, `mProjectPP` and optional `mProjectQL` for rotated TAN reprojection, plus gzip input and Galactic HEALPix reprojection with `mProject`
- `mOverlaps`, `mDiffExec`, `mFitExec`, `mBgModel` and `mBgExec` for all three image pairs and level and slope background correction
- `mAdd` for mean, median, count and sum mosaics with area images
- `mShrink`, `mSubimage`, `mExamine`, `mFixNaN` and `mConvert` for shrinking, sky and pixel cutouts, statistics, NaN handling, integer and floating-point output
- `mViewer` for a fixed-range grayscale PNG without fonts or overlays
- `mHPXHdr` and `mHPXCoord2Tile` for a level-zero header and three sky positions at orders 5 and 12

`mHdr` is skipped because it requests a header over the network even for numeric coordinates. The script writes its own headers instead. `mProjectQL` is skipped only when neither build installs it. If just one build has it, setup fails. Its optional `-L` mode is not used because it gave different pixels between two runs of the same build. The default nearest-neighbor mode is used instead.

## Normalization and limits

Only elapsed time in stdout status lines is normalized. The final numeric `time=` field from `mProject`, `mProjectPP`, `mProjectQL`, `mAdd` and `mShrink` becomes `time=<elapsed>`. Stderr, FITS headers, pixel bytes, padding, tables, filenames and PNG bytes are not normalized. There is no floating-point tolerance.

Both builds use the same relative paths. Explicit image lists fix input order, `LC_ALL=C` fixes locale, `CDPATH` is unset, and `gzip -n` omits filename and modification time. Each `mMakeImg` invocation starts from the C library's default `rand()` sequence because it never calls `srand()`.

Compare on one machine with the same compiler, runtime libraries and build settings. Noise sequences can differ between C libraries, and floating-point results can differ between machines and compilers. This is not a cross-platform reproducibility test.

FITS differences get a primary-header card diff. If Python 3 and numpy are available, an optional report counts differing pixels and gives the maximum absolute difference in scaled units. Otherwise the shell reports whether pixel data bytes differ. These diagnostics never change the byte comparison result. The pixel reader handles the small primary-image formats in this pipeline, not arbitrary FITS extensions. The HEALPix template is 2560 by 2560 and may require arrays of that size even though its output is trimmed.

## CI

The `Output comparison (Ubuntu 24.04)` job runs on pull requests. It builds the PR merge commit and the PR base commit on the same runner, then uses the merge commit's script to compare them. A PR meant to change output will fail this job, and the log lists the files that differ. This separate job does not change `make test`, which needs only one build.
