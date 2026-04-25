#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--include", required=True)
    args = parser.parse_args()

    include_root = pathlib.Path(args.include)
    headers = sorted(include_root.glob("hanon_trainer/**/*.h"))
    with tempfile.TemporaryDirectory(prefix="ht-header-check-") as temporary:
        temp_root = pathlib.Path(temporary)
        for header in headers:
            relative = header.relative_to(include_root)
            source = temp_root / f"{relative.as_posix().replace('/', '_')}.c"
            output = temp_root / f"{source.stem}.o"
            source.write_text(f"#include <{relative.as_posix()}>\nint main(void) {{ return 0; }}\n",
                              encoding="utf-8")
            command = [
                args.compiler,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                f"-I{include_root}",
                "-c",
                str(source),
                "-o",
                str(output),
            ]
            completed = subprocess.run(command, text=True, capture_output=True, check=False)
            if completed.returncode != 0:
                print(f"header independence failed for {relative}")
                print(completed.stdout)
                print(completed.stderr)
                return completed.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
