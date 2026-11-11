"""Check the package version with python check_version.py [--tag MontagePy-v<version>]."""

import argparse
from pathlib import Path
import tomllib

# Historical MontagePy 2.3.1 wheels were distributed from
# built_wheels_v2.3.1 in this repository. Rebuilt releases must be
# strictly newer so pip can upgrade those installations.
RELEASE_VERSION_FLOOR = (2, 3, 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", help="Release tag to compare with the package version")
    args = parser.parse_args()

    path = Path(__file__).resolve().parent / "pyproject.toml"
    try:
        with path.open("rb") as source:
            version = tomllib.load(source)["project"]["version"]
    except (OSError, KeyError, TypeError, ValueError) as error:
        # ValueError covers malformed TOML and bytes that are not UTF-8.
        parser.error(f"Cannot read project version from {path}: {error}")
    if not isinstance(version, str) or not version:
        parser.error(f"Project version in {path} must be a nonempty string")

    if args.tag is not None:
        prefix = "MontagePy-v"
        if not args.tag.startswith(prefix):
            parser.error(f"Tag {args.tag!r} must start with {prefix!r}")
        if args.tag[len(prefix):] != version:
            parser.error(f"Tag {args.tag!r} does not match package version {version!r}")
        parts = version.split(".")
        if len(parts) != 3 or not all(
            part.isascii() and part.isdigit() for part in parts
        ):
            parser.error(f"Version {version!r} must be a numeric x.y.z release")
        if tuple(int(part) for part in parts) <= RELEASE_VERSION_FLOOR:
            floor = ".".join(str(part) for part in RELEASE_VERSION_FLOOR)
            parser.error(
                f"Version {version!r} must be newer than {floor!r}: "
                f"historical {floor} wheels were distributed from this repository "
                "and pip needs a newer version to upgrade those installations"
            )

    print(version)


if __name__ == "__main__":
    main()
