#!/usr/bin/env python3
import argparse
import pathlib
import subprocess


def raster_start(data: bytes) -> int:
    offset = -1
    for _ in range(3):
        offset = data.find(b"\n", offset + 1)
        if offset < 0:
            raise AssertionError("missing PPM header newline")
    return offset + 1


def assert_snapshot(path: pathlib.Path) -> None:
    data = path.read_bytes()
    assert data.startswith(b"P6\n8 8\n255\n")
    raster = data[raster_start(data):]
    assert len(raster) == 8 * 8 * 3
    assert any(value != raster[0] for value in raster)
    assert b"\xff\x00\x00" in raster


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--viewer", required=True)
    parser.add_argument("--fixtures", required=True)
    parser.add_argument("--out-dir", required=True)
    args = parser.parse_args()

    viewer = pathlib.Path(args.viewer)
    fixtures = pathlib.Path(args.fixtures)
    out_dir = pathlib.Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    snapshot = out_dir / "viewer-valid.ppm"
    ok = subprocess.run(
        [
            str(viewer),
            "--corpus",
            str(fixtures),
            "--asset-root",
            str(fixtures),
            "--variant",
            "viewer-valid",
            "--step",
            "0",
            "--viewport",
            "8x8",
            "--out",
            str(snapshot),
        ],
        check=False,
        text=True,
        capture_output=True,
    )
    if ok.returncode != 0:
        print(ok.stdout)
        print(ok.stderr)
        return ok.returncode
    assert "variant=viewer-valid" in ok.stdout
    assert "render=HT_OK" in ok.stdout
    assert_snapshot(snapshot)

    missing = subprocess.run(
        [
            str(viewer),
            "--corpus",
            str(fixtures),
            "--asset-root",
            str(fixtures),
            "--variant",
            "viewer-missing-asset",
            "--step",
            "0",
            "--viewport",
            "8x8",
            "--out",
            str(out_dir / "missing.ppm"),
        ],
        check=False,
        text=True,
        capture_output=True,
    )
    if missing.returncode != 3:
        print(missing.stdout)
        print(missing.stderr)
        return 1
    assert "asset_status=HT_ERR_NOT_FOUND" in missing.stdout
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
