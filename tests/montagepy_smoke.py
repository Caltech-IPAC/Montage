"""Regression test for an installed MontagePy wheel, using only the stdlib.

Check bundled CFITSIO, mMakeImg call isolation, mExamine dimensions, known
image values, and that positional arguments in the Python signature order
reach the library correctly. Run a synthetic image and cube pipeline twice with
junk-filled allocations and once in a fresh process without memory filling.
Require successful calls and identical results and output digests.
Reported run times are left out of results. FITS outputs are compared by
image dimensions and pixel values only, treating NaNs as equal and -0 as 0.
Other outputs are compared in full. Temporary paths are normalised throughout.

The script re-execs with MALLOC_PERTURB_=165 (glibc) and MallocScribble=1
(macOS), before importing MontagePy. The private child mode skips that step.
Network services and mTANHdr, mBestImage and mShrinkCube are deliberately
outside this regression test.

Usage, after building and installing the wheel (python/MontagePy/make_local.sh):
    python tests/montagepy_smoke.py
Run it from outside python/MontagePy so the installed package is imported.
"""

import ctypes
import hashlib
import json
import math
import os
import struct
import subprocess
import sys
import tempfile
import time

CHILD_MODE = len(sys.argv) == 3 and sys.argv[1] == "--pipeline-child"
FILL_ENV = {"MALLOC_PERTURB_": "165", "MallocScribble": "1"}
if not CHILD_MODE and any(os.environ.get(k) != v for k, v in FILL_ENV.items()):
    os.execve(sys.executable, [sys.executable] + sys.argv,
              dict(os.environ, **FILL_ENV))

import MontagePy.main as m
import MontagePy._wrappers as wrappers

# CFITSIO reports major + minor/100 + micro/10000, so 4.7.0 is 4.07.
CFITSIO_VERSION = 4.07

HEADER = """SIMPLE  =                    T
BITPIX  =                  -64
NAXIS   =                    2
NAXIS1  =                   50
NAXIS2  =                   50
CTYPE1  = 'RA---TAN'
CTYPE2  = 'DEC--TAN'
EQUINOX =               2000.0
CRVAL1  =                 10.0
CRVAL2  =                 20.0
CRPIX1  =                 25.5
CRPIX2  =                 25.5
CDELT1  =               -0.001
CDELT2  =                0.001
{extra}END
"""

failures = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else ": " + str(detail)),
          flush=True)
    if not ok:
        failures.append(name)


def card(key, value):
    return ("%-8s= %20s" % (key, value)).ljust(80)


def image_cards(n=50, ra=10.0, planes=None):
    cards = [("SIMPLE", "T"), ("BITPIX", "-64"),
             ("NAXIS", "2" if planes is None else "3"),
             ("NAXIS1", str(n)), ("NAXIS2", str(n))]
    if planes is not None:
        cards.append(("NAXIS3", str(len(planes))))
    cards += [("CTYPE1", "'RA---TAN'"), ("CTYPE2", "'DEC--TAN'"),
              ("EQUINOX", "2000.0"), ("CDELT1", "-0.001"),
              ("CDELT2", "0.001"), ("CRVAL1", str(ra)), ("CRVAL2", "20.0"),
              ("CRPIX1", str((n + 1) / 2)), ("CRPIX2", str((n + 1) / 2))]
    if planes is not None:
        cards += [("CTYPE3", "'VELO'"), ("CRVAL3", "0.0"),
                  ("CRPIX3", "1.0"), ("CDELT3", "1.0")]
    return cards


def write_header(path, n=50, ra=10.0, extra=(), planes=None):
    with open(path, "w") as f:
        f.write("\n".join(card(k, v).rstrip()
                          for k, v in image_cards(n, ra, planes) + list(extra)) + "\nEND\n")


def write_image(path, n=50, planes=None):
    """Write constant image data without needing a working mMakeImg."""
    header = "".join(card(k, v) for k, v in image_cards(n, planes=planes))
    header += "END".ljust(80)
    header = header.ljust((len(header) + 2879) // 2880 * 2880).encode("ascii")
    values = (5.0,) if planes is None else planes
    data = b"".join(struct.pack(">d", v) * (n * n) for v in values)
    with open(path, "wb") as f:
        f.write(header + data + b"\0" * (-len(data) % 2880))


def read_image(path):
    """Return the axis lengths and pixel values of the primary FITS array."""
    with open(path, "rb") as f:
        data = f.read()
    cards = {}
    for offset in range(0, len(data), 80):
        text = data[offset:offset + 80].decode("ascii")
        if text[:8].strip() == "END":
            break
        if text[8:10] == "= ":
            cards[text[:8].strip()] = text[10:].split("/", 1)[0].strip()
    else:
        raise ValueError("FITS header has no END card")
    bitpix = int(cards["BITPIX"])
    codes = {8: "B", 16: "h", 32: "i", 64: "q", -32: "f", -64: "d"}
    axes = tuple(int(cards["NAXIS%d" % i])
                 for i in range(1, int(cards["NAXIS"]) + 1))
    count = math.prod(axes)
    start = (offset // 2880 + 1) * 2880
    size = count * abs(bitpix) // 8
    if len(data) < start + size:
        raise ValueError("Truncated FITS array")
    scale = float(cards.get("BSCALE", "1"))
    zero = float(cards.get("BZERO", "0"))
    values = [v[0] * scale + zero
              for v in struct.iter_unpack(">" + codes[bitpix], data[start:start + size])]
    return axes, values


def centre_pixel(path):
    """Pixel (25, 25) of a 2-D FITS image, read with the stdlib."""
    axes, values = read_image(path)
    return values[25 * axes[0] + 25]


def close(value, expected):
    return math.isfinite(value) and abs(value - expected) <= 1e-6 * max(1.0, abs(expected))


def image_answer(path, expected, filled=False):
    """Check every pixel value, including each separate cube plane.

    NaN pixels are skipped, unless filled is true and none may be NaN."""
    axes, values = read_image(path)
    planes = expected if isinstance(expected, tuple) else (expected,)
    if len(planes) > 1 and (len(axes) != 3 or axes[2] != len(planes)):
        return {"ok": False, "shape": axes, "expected_planes": planes}
    size = len(values) // len(planes)
    bounds = []
    ok = True
    for i, value in enumerate(planes):
        plane = values[i * size:(i + 1) * size]
        pixels = [v for v in plane if filled or not math.isnan(v)]
        bounds.append([min(pixels), max(pixels)] if pixels else [])
        ok = ok and bool(pixels) and all(close(v, value) for v in pixels)
    return {"ok": ok, "bounds": bounds, "expected": planes}


def normalise(value, directory):
    if isinstance(value, bytes):
        value = value.decode("ascii", errors="replace")
    if isinstance(value, str):
        return value.replace(directory, "<TMP>")
    if isinstance(value, float):
        if math.isnan(value):
            return "<NaN>"
        if math.isinf(value):
            return "<+Infinity>" if value > 0 else "<-Infinity>"
    if isinstance(value, dict):
        return {k: normalise(v, directory) for k, v in value.items() if k != "time"}
    if isinstance(value, (list, tuple)):
        return [normalise(v, directory) for v in value]
    return value


def digest(path, directory):
    if path.endswith((".fits", ".fit")):
        axes, values = read_image(path)
        h = hashlib.sha256(str(axes).encode("ascii"))
        for value in values:
            # Canonicalise NaNs and signed zero before hashing numerical data.
            h.update(struct.pack(">d", math.nan if math.isnan(value) else value + 0.0))
        return h.hexdigest()
    with open(path, "rb") as f:
        data = f.read().replace(directory.encode(), b"<TMP>")
    return hashlib.sha256(data).hexdigest()


def baseline(directory):
    version = ctypes.c_float()
    ctypes.CDLL(wrappers.__file__).ffvers(ctypes.byref(version))
    check("bundled CFITSIO %.2f" % CFITSIO_VERSION,
          round(version.value, 2) == CFITSIO_VERSION, version.value)

    known = os.path.join(directory, "known.fits")
    write_image(known)
    # Read a cube first so the 2-D call must clear its unused dimensions.
    cube = os.path.join(directory, "known-cube.fits")
    write_image(cube, n=30, planes=(1.0, 2.0, 3.0))
    result = m.mExamine(cube)
    check("mExamine cube setup", result.get("status") == "0" and result.get("naxis3") == 3,
          result)
    result = m.mExamine(known)
    check("mExamine 2-D status", result.get("status") == "0", result)
    check("mExamine 2-D naxis3/naxis4",
          result.get("naxis3") == 0 and result.get("naxis4") == 0,
          {k: result.get(k) for k in ("naxis3", "naxis4")})
    check("mExamine 2-D known dimensions",
          result.get("naxis1") == 50 and result.get("naxis2") == 50, result)
    check("mExamine constant value", close(result.get("aveflux", math.nan), 5.0), result)

    hdr = os.path.join(directory, "in.hdr")
    rotated = os.path.join(directory, "rot.hdr")
    with open(hdr, "w") as f:
        f.write(HEADER.format(extra=""))
    # Same field, rotated 30 degrees: mProject must really resample.
    with open(rotated, "w") as f:
        f.write(HEADER.format(extra="CROTA2  =                 30.0\n"))

    def make_image(name, path, expected, *args):
        result = m.mMakeImg(hdr, path, *args)
        check(name + " status", result.get("status") == "0", result)
        if result.get("status") == "0":
            value = centre_pixel(path)
            check(name + " value", close(value, expected),
                  "centre pixel %r, expected %r" % (value, expected))

    background = os.path.join(directory, "background.fits")
    make_image("mMakeImg command mode -b 5", background, 5.0, "-b 5 5 5 5", 2)
    make_image("mMakeImg defaults", os.path.join(directory, "default.fits"), 0.0)
    make_image("mMakeImg command mode no options",
               os.path.join(directory, "empty.fits"), 0.0, "", 2)
    make_image("mMakeImg JSON mode bg 2", os.path.join(directory, "json.fits"), 2.0,
               '{"background": {"bg11": 2, "bg1N": 2, "bgNN": 2, "bgN1": 2}}', 1)
    # A JSON call that sets nothing must not see values from earlier calls.
    nothing = "{}"

    def leave_values(name, path, layout, mode):
        """Call mMakeImg with options, and check that they changed the image."""
        result = m.mMakeImg(hdr, path, layout, mode)
        check(name + " status", result.get("status") == "0", result)
        if result.get("status") == "0":
            value = centre_pixel(path)
            check(name + " changes the image", not close(value, 0.0),
                  "centre pixel %r" % value)

    make_image("mMakeImg JSON mode empty after JSON bg",
               os.path.join(directory, "json-empty.fits"), 0.0, nothing, 1)
    leave_values("mMakeImg command mode -n -b", os.path.join(directory, "noisy.fits"),
                 "-n 1 -b 5 5 5 5", 2)
    make_image("mMakeImg JSON mode empty after command mode",
               os.path.join(directory, "json-empty2.fits"), 0.0, nothing, 1)
    leave_values("mMakeImg JSON mode noise", os.path.join(directory, "json-noisy.fits"),
                 '{"background": {"noise": 1}}', 1)
    make_image("mMakeImg JSON mode empty after JSON noise",
               os.path.join(directory, "json-empty3.fits"), 0.0, nothing, 1)

    projected = os.path.join(directory, "projected.fits")
    result = m.mProject(background, projected, rotated)
    check("mProject status", result.get("status") == "0", result)
    if result.get("status") == "0":
        value = centre_pixel(projected)
        check("mProject keeps the value", close(value, 5.0),
              "centre pixel %r, expected 5.0" % value)

    def call(fn, *args, **kwargs):
        try:
            return fn(*args, **kwargs)
        except Exception as e:
            return {"status": "exception", "msg": repr(e)}

    # Python signatures put required arguments first. Pass them by position
    # so a wrapper that forwards them in another order is caught.
    tiles = os.path.join(directory, "tile")
    result = call(m.mTileImage, background, tiles, 2, 2)
    check("mTileImage status",
          result.get("status") == "0" and result.get("ntile") == 4
          and result.get("nfailed") == 0, result)
    # Only the first tile has a fixed shape. mSubimage pixel mode adds a
    # pixel to cutouts that do not start at the image edge.
    for i in (1, 2):
        for k in (1, 2):
            tile = "%s_%d_%d.fits" % (tiles, i, k)
            try:
                axes, _ = read_image(tile)
                answer = image_answer(tile, 5.0)
            except (OSError, ValueError) as e:
                axes, answer = None, {"ok": False, "error": str(e)}
            shape_ok = axes == (25, 25) if (i, k) == (1, 1) else axes is not None
            check("mTileImage tile %d %d" % (i, k),
                  shape_ok and answer["ok"], {"axes": axes, **answer})

    # A constant image has no value range to measure, so use a gradient
    # whose minimum and maximum are the corner values.
    gradient = os.path.join(directory, "gradient.fits")
    result = m.mMakeImg(hdr, gradient, "-b 1 1 9 9", 2)
    check("mMakeImg gradient status", result.get("status") == "0", result)
    result = call(m.mHistogram, gradient, os.path.join(directory, "hist.txt"),
                  "0%", "100%", "linear")
    check("mHistogram status", result.get("status") == "0", result)
    check("mHistogram data range",
          close(result.get("datamin", math.nan), 1.0)
          and close(result.get("datamax", math.nan), 9.0), result)

    # mode=2 takes a command string. The default mode reads a JSON file.
    # parsecmd splits command strings on spaces and commas outside quotes.
    png = os.path.join(directory, "view.png")
    result = call(m.mViewer, "-ct 1 -gray \"%s\" 0%% 100%% linear" % gradient, png, mode=2)
    check("mViewer status", result.get("status") == "0", result)
    try:
        with open(png, "rb") as f:
            magic = f.read(8)
    except OSError as e:
        magic = str(e)
    check("mViewer writes a PNG", magic == b"\x89PNG\r\n\x1a\n", magic)


def pipeline(directory):
    out = {}

    def j(*parts):
        return os.path.join(directory, *parts)

    def rec(name, fn, *args, files=(), answers=None, filled=(), **kwargs):
        entry = {"files": {}, "answers": {}, "errors": []}
        try:
            entry["result"] = normalise(fn(*args, **kwargs), directory)
        except Exception as e:
            entry["result"] = {"status": "exception", "msg": normalise(str(e), directory)}
        for path in files:
            try:
                entry["files"][path] = digest(j(path), directory)
            except Exception as e:
                entry["errors"].append(normalise("%s: %s" % (path, e), directory))
        for path, expected in (answers or {}).items():
            try:
                entry["answers"][path] = normalise(
                    image_answer(j(path), expected, path in filled), directory)
            except Exception as e:
                entry["errors"].append(normalise("%s: %s" % (path, e), directory))
        out[name] = entry

    for sub in ("raw", "proj", "diff", "corr", "cubes", "cproj"):
        os.makedirs(j(sub))
    write_header(j("a.hdr"))
    write_header(j("b.hdr"), ra=10.03)
    write_header(j("mosaic.hdr"), n=90, ra=10.015, extra=[("CROTA2", "10.0")])
    write_header(j("cube.hdr"), planes=(1.0, 2.0, 3.0))
    write_image(j("cubes", "c.fits"), n=30, planes=(1.0, 2.0, 3.0))
    with open(j("box.txt"), "w") as f:
        f.write("40 45 40 45\n")

    rec("mMakeImg a", m.mMakeImg, j("a.hdr"), j("raw", "a.fits"), "-b 4 4 4 4", 2,
        files=["raw/a.fits"], answers={"raw/a.fits": 4.0}, filled=["raw/a.fits"])
    rec("mMakeImg b", m.mMakeImg, j("b.hdr"), j("raw", "b.fits"), "-b 4 4 4 4", 2,
        files=["raw/b.fits"], answers={"raw/b.fits": 4.0}, filled=["raw/b.fits"])
    rec("mImgtbl raw", m.mImgtbl, j("raw"), j("raw.tbl"), files=["raw.tbl"])
    rec("mProject", m.mProject, j("raw", "a.fits"), j("pa.fits"), j("mosaic.hdr"),
        files=["pa.fits", "pa_area.fits"], answers={"pa.fits": 4.0})
    rec("mProjectPP", m.mProjectPP, j("raw", "a.fits"), j("ppa.fits"), j("mosaic.hdr"),
        files=["ppa.fits", "ppa_area.fits"], answers={"ppa.fits": 4.0})
    rec("mProjectQL", m.mProjectQL, j("raw", "a.fits"), j("qla.fits"), j("mosaic.hdr"),
        files=["qla.fits"], answers={"qla.fits": 4.0})
    rec("mProjExec", m.mProjExec, j("raw"), j("raw.tbl"), j("mosaic.hdr"), projdir=j("proj"),
        files=["proj/hdu0_a.fits", "proj/hdu0_b.fits",
               "proj/hdu0_a_area.fits", "proj/hdu0_b_area.fits"],
        answers={"proj/hdu0_a.fits": 4.0, "proj/hdu0_b.fits": 4.0})
    rec("mImgtbl proj", m.mImgtbl, j("proj"), j("proj.tbl"), files=["proj.tbl"])
    rec("mOverlaps", m.mOverlaps, j("proj.tbl"), j("diffs.tbl"), files=["diffs.tbl"])
    rec("mDiff", m.mDiff, j("proj", "hdu0_a.fits"), j("proj", "hdu0_b.fits"),
        j("d.fits"), j("mosaic.hdr"), files=["d.fits", "d_area.fits"])
    rec("mFitplane", m.mFitplane, j("d.fits"))
    rec("mDiffFit", m.mDiffFit, j("proj", "hdu0_a.fits"), j("proj", "hdu0_b.fits"),
        j("df.fits"), j("mosaic.hdr"), keep=True, files=["df.fits", "df_area.fits"])
    rec("mDiffExec", m.mDiffExec, j("proj"), j("diffs.tbl"), j("mosaic.hdr"), diffdir=j("diff"))
    rec("mFitExec", m.mFitExec, j("diffs.tbl"), j("fits.tbl"), diffdir=j("diff"), files=["fits.tbl"])
    rec("mDiffFitExec", m.mDiffFitExec, j("proj"), j("diffs.tbl"), j("mosaic.hdr"),
        j("diff"), j("fits2.tbl"), files=["fits2.tbl"])
    rec("mBgModel", m.mBgModel, j("proj.tbl"), j("fits.tbl"), j("corr.tbl"), files=["corr.tbl"])
    rec("mBackground", m.mBackground, j("proj", "hdu0_a.fits"), j("bga.fits"), 0.0, 0.0, 1.0,
        files=["bga.fits", "bga_area.fits"], answers={"bga.fits": 3.0})
    rec("mBgExec", m.mBgExec, j("proj"), j("proj.tbl"), j("corr.tbl"), j("corr"),
        files=["corr/hdu0_a.fits", "corr/hdu0_b.fits",
               "corr/hdu0_a_area.fits", "corr/hdu0_b_area.fits"])
    rec("mAdd", m.mAdd, j("corr"), j("proj.tbl"), j("mosaic.hdr"), j("mosaic.fits"),
        files=["mosaic.fits", "mosaic_area.fits"], answers={"mosaic.fits": 4.0})
    rec("mAddMem", m.mAddMem, j("corr"), j("proj.tbl"), j("mosaic.hdr"), j("mosaic2.fits"),
        files=["mosaic2.fits", "mosaic2_area.fits"], answers={"mosaic2.fits": 4.0})
    rec("mExamine", m.mExamine, j("mosaic.fits"))
    rec("mCoord", m.mCoord, j("mosaic.fits"), ra=10.015, dec=20.0)
    rec("mCoverageCheck", m.mCoverageCheck, j("raw.tbl"), j("cov.tbl"), 3, "", 2,
        [10.0, 20.0], files=["cov.tbl"])
    rec("mSubset", m.mSubset, j("raw.tbl"), j("a.hdr"), j("subset.tbl"), files=["subset.tbl"])
    rec("mMakeHdr", m.mMakeHdr, j("raw.tbl"), j("made.hdr"), files=["made.hdr"])
    rec("mGetHdr", m.mGetHdr, j("mosaic.fits"), j("got.hdr"), files=["got.hdr"])
    rec("mPutHdr", m.mPutHdr, j("mosaic.fits"), j("put.fits"), j("got.hdr"), files=["put.fits"])
    rec("mSubHdr", m.mSubHdr, j("a.hdr"), j("sub.hdr"), 10.0, 20.0, "0.02", "0.02", files=["sub.hdr"])
    rec("mSubimage", m.mSubimage, j("mosaic.fits"), j("cut.fits"), 10.015, 20.0, 0.02, 0.02,
        files=["cut.fits"], answers={"cut.fits": 4.0})
    rec("mShrink", m.mShrink, j("mosaic.fits"), j("small.fits"), 3, files=["small.fits"],
        answers={"small.fits": 4.0})
    rec("mFlatten", m.mFlatten, j("mosaic.fits"), j("flat.fits"), files=["flat.fits"])
    rec("mFixNaN", m.mFixNaN, j("mosaic.fits"), j("fixed.fits"), True, -1.0,
        0, [], [], [], [], files=["fixed.fits"])
    rec("mMask", m.mMask, j("mosaic.fits"), j("masked.fits"), j("box.txt"), files=["masked.fits"])
    rec("mExamine cube", m.mExamine, j("cubes", "c.fits"))
    rec("mProjectCube", m.mProjectCube, j("cubes", "c.fits"), j("cproj", "c.fits"), j("a.hdr"),
        files=["cproj/c.fits", "cproj/c_area.fits"], answers={"cproj/c.fits": (1.0, 2.0, 3.0)})
    rec("mImgtbl cube", m.mImgtbl, j("cproj"), j("cube.tbl"), files=["cube.tbl"])
    rec("mAddCube", m.mAddCube, j("cproj"), j("cube.tbl"), j("cube.hdr"), j("cadd.fits"),
        files=["cadd.fits", "cadd_area.fits"], answers={"cadd.fits": (1.0, 2.0, 3.0)})
    rec("mSubCube", m.mSubCube, 0, j("cubes", "c.fits"), j("csub.fits"), 10.0, 20.0, 0.01, 0.01,
        d3constraint="1,2", files=["csub.fits"], answers={"csub.fits": (1.0, 2.0)})
    rec("mTranspose", m.mTranspose, j("cubes", "c.fits"), j("ctr.fits"), 3, [3, 1, 2], files=["ctr.fits"])
    return out


def compare_pipelines():
    # A file keeps C library stdout from interfering with the child's JSON.
    with tempfile.TemporaryDirectory() as tmp:
        result_path = os.path.join(tmp, "child.json")
        env = {k: v for k, v in os.environ.items() if k not in FILL_ENV}
        try:
            child = subprocess.run([sys.executable, os.path.abspath(__file__),
                                    "--pipeline-child", result_path],
                                   env=env, capture_output=True, text=True, timeout=45)
            check("pipeline fresh process", child.returncode == 0,
                  "exit %s\n%s\n%s" % (child.returncode, child.stdout, child.stderr))
            if child.returncode != 0:
                return
            with open(result_path) as f:
                fresh = json.load(f)
        except (OSError, ValueError, subprocess.TimeoutExpired) as e:
            check("pipeline fresh process", False, e)
            return

    runs = [fresh]
    for _ in range(2):
        with tempfile.TemporaryDirectory() as tmp:
            runs.append(pipeline(os.path.realpath(tmp)))
    check("pipeline call list", runs[0].keys() == runs[1].keys() == runs[2].keys())
    for name in fresh:
        entries = [run.get(name, {}) for run in runs]
        statuses = [e.get("result", {}).get("status") for e in entries]
        errors = [e.get("errors") for e in entries]
        check("pipeline " + name + " status", statuses == ["0"] * 3 and not any(errors),
              {"results": [e.get("result") for e in entries], "output_errors": errors})
        if any(e.get("answers") for e in entries):
            check("pipeline " + name + " known values",
                  all(a["ok"] for e in entries for a in e.get("answers", {}).values()),
                  [e.get("answers") for e in entries])
        keys = sorted(set().union(*(e.keys() for e in entries)))
        differences = {key: [e.get(key) for e in entries]
                       for key in keys if not entries[0].get(key) == entries[1].get(key) == entries[2].get(key)}
        check("pipeline " + name + " repeatability", not differences,
              json.dumps(differences, sort_keys=True))


def main():
    if CHILD_MODE:
        with tempfile.TemporaryDirectory() as tmp:
            result = pipeline(os.path.realpath(tmp))
        with open(sys.argv[2], "w") as f:
            json.dump(result, f, allow_nan=False)
        return 0

    started = time.monotonic()
    with tempfile.TemporaryDirectory() as tmp:
        baseline(os.path.realpath(tmp))
    compare_pipelines()
    print("python %s, MontagePy from %s" % (sys.version.split()[0], os.path.dirname(m.__file__)))
    print("elapsed %.2f s" % (time.monotonic() - started))
    if failures:
        print("%d check(s) failed" % len(failures))
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
