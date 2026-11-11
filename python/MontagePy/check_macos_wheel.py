"""Check extension architectures with python check_macos_wheel.py wheelhouse/*.whl."""

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory
import zipfile
import zlib


def check_wheel(wheel):
    if wheel.suffix != ".whl" or "-" not in wheel.name:
        print(f"{wheel}: Expected a wheel filename", file=sys.stderr)
        return False

    platform_tags = wheel.stem.rsplit("-", 1)[1].split(".")
    architectures = set()
    for tag in platform_tags:
        match = re.fullmatch(r"macosx_[0-9]+_[0-9]+_(arm64|x86_64)", tag)
        if match is None:
            print(f"{wheel}: Unsupported platform tag {tag!r}", file=sys.stderr)
            return False
        architectures.add(match.group(1))
    if len(architectures) != 1:
        print(f"{wheel}: Mixed platform architectures", file=sys.stderr)
        return False
    expected = architectures.pop()

    valid = True
    try:
        with zipfile.ZipFile(wheel) as archive, TemporaryDirectory() as temporary:
            extensions = [
                member for member in archive.infolist()
                if member.filename.endswith(".so")
            ]
            if not extensions:
                print(f"{wheel}: No compiled extensions found", file=sys.stderr)
                return False
            for index, member in enumerate(extensions):
                try:
                    path = Path(temporary) / f"{index}.so"
                    with archive.open(member) as source, path.open("wb") as target:
                        shutil.copyfileobj(source, target)
                    result = subprocess.run(
                        ["lipo", "-archs", str(path)],
                        capture_output=True,
                        text=True,
                        check=False,
                    )
                except (OSError, RuntimeError, zipfile.BadZipFile, zlib.error) as error:
                    # zipfile raises RuntimeError for encrypted members.
                    print(f"{wheel}: {member.filename}: {error}", file=sys.stderr)
                    valid = False
                    continue
                actual = result.stdout.split()
                print(f"{wheel}: {member.filename}: {' '.join(actual) or '<none>'}")
                if result.returncode != 0:
                    print(
                        f"{wheel}: {member.filename}: lipo failed with status "
                        f"{result.returncode}: {result.stderr.strip()}",
                        file=sys.stderr,
                    )
                    valid = False
                elif actual != [expected]:
                    print(
                        f"{wheel}: {member.filename}: Expected only {expected}",
                        file=sys.stderr,
                    )
                    valid = False
    except (OSError, RuntimeError, zipfile.BadZipFile) as error:
        # Unsupported ZIP versions raise NotImplementedError, a RuntimeError
        # subclass.
        print(f"{wheel}: {error}", file=sys.stderr)
        return False
    return valid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wheels", nargs="+", type=Path, help="macOS wheel paths")
    args = parser.parse_args()
    valid = True
    for wheel in args.wheels:
        if not check_wheel(wheel):
            valid = False
    return 0 if valid else 1


if __name__ == "__main__":
    sys.exit(main())
