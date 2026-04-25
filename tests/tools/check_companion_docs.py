#!/usr/bin/env python3
import argparse
import pathlib


REQUIRED_SECTIONS = [
    "## Purpose",
    "## Theory",
    "## Architecture Role",
    "## Implementation Contract",
    "## Ownership And Failure Modes",
    "## Test Strategy",
    "## Spec Links",
]


def source_files(root: pathlib.Path) -> list[pathlib.Path]:
    files: list[pathlib.Path] = []
    files.extend(sorted((root / "include").glob("hanon_trainer/**/*.h")))
    files.extend(sorted((root / "src").glob("*.c")))
    files.extend(sorted((root / "tools").glob("*.c")))
    files.extend(sorted((root / "tests").glob("*.c")))
    return files


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    args = parser.parse_args()

    root = pathlib.Path(args.root)
    failures: list[str] = []
    for source in source_files(root):
        doc = pathlib.Path(f"{source}.md")
        if not doc.exists():
            failures.append(f"missing companion doc: {doc.relative_to(root)}")
            continue
        text = doc.read_text(encoding="utf-8")
        for section in REQUIRED_SECTIONS:
            if section not in text:
                failures.append(f"{doc.relative_to(root)} missing {section}")
        if ("docs/LOCAL_SPEC.md" not in text) and ("docs/ARCHITECTURE.md" not in text) \
                and ("docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md" not in text):
            failures.append(f"{doc.relative_to(root)} lacks a project spec link")

    if failures:
        for failure in failures:
            print(failure)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
