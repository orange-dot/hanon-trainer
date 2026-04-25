#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import tempfile


EX_USAGE = 64


def run_case(executable: str, args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run([executable, *args], text=True, capture_output=True, check=False)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", required=True)
    args = parser.parse_args()

    completed = run_case(args.probe, ["--help"])
    require(completed.returncode == 0, "--help should exit 0")
    require("Usage:" in completed.stdout, "--help should print usage on stdout")

    completed = run_case(args.probe, ["--version"])
    require(completed.returncode == 0, "--version should exit 0")
    require("hanon-trainer" in completed.stdout, "--version should print project name")
    require("ALSA" in completed.stdout, "--version should print ALSA runtime version")

    completed = run_case(args.probe, ["--list"])
    require(completed.returncode in (0, 1), "--list should succeed or report ALSA runtime failure")
    if completed.returncode == 0:
        require("port\tclient\tname" in completed.stdout, "--list success should print TSV-like header")
    else:
        require("alsa:" in completed.stderr, "--list runtime failure should be reported on stderr")

    completed = run_case(args.probe, ["--duration", "99999", "--port", "24:0", "--format", "table"])
    require(completed.returncode == EX_USAGE, "duration above upper bound should be usage error")

    completed = run_case(args.probe, ["--port"])
    require(completed.returncode == EX_USAGE, "missing --port value should be usage error")

    completed = run_case(args.probe, ["--port", "24:0", "--duration", "1", "--format", "json"])
    require(completed.returncode == EX_USAGE, "invalid format should be usage error")

    completed = run_case(args.probe, ["--port", "not-a-port:0", "--duration", "1", "--format", "table"])
    require(completed.returncode == 1, "malformed or unavailable ALSA port should be runtime error")
    require("alsa:" in completed.stderr, "runtime ALSA failure should be reported on stderr")

    with tempfile.TemporaryDirectory() as tmp_dir:
        raw_tsv = pathlib.Path(tmp_dir) / "raw.tsv"
        raw_tsv.write_text("ms\traw\n0\t90 3C 40\n5\t90 3C 00\n", encoding="utf-8")
        completed = run_case(args.probe, ["--replay-raw-tsv", str(raw_tsv), "--format", "tsv"])
        require(completed.returncode == 0, "raw replay should not require ALSA hardware")
        require("0\tnote_on\t1\t60\t64\t\t\t0x90\n" in completed.stdout,
                "raw replay should decode note-on")
        require("5\tnote_off\t1\t60\t0\t\t\t0x90\n" in completed.stdout,
                "raw replay should normalize NOTEON velocity zero")

        completed = run_case(
            args.probe,
            ["--replay-raw-tsv", str(raw_tsv), "--port", "24:0", "--format", "tsv"],
        )
        require(completed.returncode == EX_USAGE, "raw replay should reject capture options")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
