#!/usr/bin/env python3
import argparse
import pathlib
import subprocess


EXPECTED_WIDTH = 2480
EXPECTED_HEIGHT = 3508


def ppm_size(path: pathlib.Path) -> tuple[int, int]:
    tokens: list[bytes] = []
    with path.open("rb") as handle:
        for line in handle:
            stripped = line.strip()
            if not stripped or stripped.startswith(b"#"):
                continue
            tokens.extend(stripped.split())
            if len(tokens) >= 4:
                break
    if len(tokens) < 4 or tokens[0] != b"P6":
        raise ValueError("not a binary PPM")
    return int(tokens[1]), int(tokens[2])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    args = parser.parse_args()

    root = pathlib.Path(args.root)
    pdf = root / "corpus/source-pdfs/hanon-exercise-01-c.pdf"
    asset = root / "corpus/runtime/assets/hanon-exercise-01-c.ppm"
    asset.parent.mkdir(parents=True, exist_ok=True)

    if not asset.exists():
        if not pdf.exists():
            print("SKIP: source PDF corpus/source-pdfs/hanon-exercise-01-c.pdf is not present")
            return 0
        prefix = asset.with_suffix("")
        completed = subprocess.run(
            ["pdftoppm", "-r", "300", "-singlefile", str(pdf), str(prefix)],
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode != 0:
            print(completed.stdout)
            print(completed.stderr)
            return completed.returncode

    width, height = ppm_size(asset)
    if (width, height) != (EXPECTED_WIDTH, EXPECTED_HEIGHT):
        print(f"unexpected PPM dimensions: {width}x{height}, expected {EXPECTED_WIDTH}x{EXPECTED_HEIGHT}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
