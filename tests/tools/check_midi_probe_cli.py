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


def parse_backend(version_stdout: str) -> str:
    for line in version_stdout.splitlines():
        if line.startswith("MIDI backend:"):
            return line.split(":", 1)[1].strip()
    raise AssertionError("--version should print MIDI backend")


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
    backend = parse_backend(completed.stdout)
    require(backend, "--version should print non-empty MIDI backend")

    completed = run_case(args.probe, ["--list"])
    require(completed.returncode in (0, 1), "--list should succeed or report MIDI runtime failure")
    if completed.returncode == 0:
        require("device_id\tbackend\tname" in completed.stdout,
                "--list success should print backend-neutral TSV header")
        if backend == "fake":
            require("fake:keyboard\tfake\tFake MIDI Keyboard" in completed.stdout,
                    "fake --list should print deterministic input")
    else:
        require("midi:" in completed.stderr, "--list runtime failure should be reported on stderr")

    completed = run_case(args.probe, ["--duration", "99999", "--port", "24:0", "--format", "table"])
    require(completed.returncode == EX_USAGE, "duration above upper bound should be usage error")

    completed = run_case(args.probe, ["--port"])
    require(completed.returncode == EX_USAGE, "missing --port value should be usage error")

    completed = run_case(args.probe, ["--port", "24:0", "--duration", "1", "--format", "json"])
    require(completed.returncode == EX_USAGE, "invalid format should be usage error")

    completed = run_case(args.probe, ["--port", "not-a-port:0", "--duration", "1", "--format", "table"])
    require(completed.returncode == 1, "malformed or unavailable MIDI port should be runtime error")
    require("midi:" in completed.stderr, "runtime MIDI failure should be reported on stderr")

    if backend == "fake":
        completed = run_case(args.probe,
                             ["--port", "fake:keyboard", "--duration", "1", "--format", "tsv"])
        require(completed.returncode == 0, "fake capture should succeed without hardware")
        require("ms\tkind\tchannel\tnote\tvelocity\tcontroller\tvalue\traw_type\n"
                in completed.stdout, "fake capture should print TSV header")
        require("0\tnote_on\t1\t60\t64\t\t\t0x90\n" in completed.stdout,
                "fake capture should include note-on")
        require("800\tsysex\t\t\t\t\t1050\t0xF0\n" in completed.stdout,
                "fake capture should report bounded oversized SysEx length")
        require("midi: captured 9 events" in completed.stderr,
                "fake capture should drain deterministic event stream")

    with tempfile.TemporaryDirectory() as tmp_dir:
        raw_tsv = pathlib.Path(tmp_dir) / "raw.tsv"
        raw_tsv.write_text("ms\traw\n0\t90 3C 40\n5\t90 3C 00\n", encoding="utf-8")
        completed = run_case(args.probe, ["--replay-raw-tsv", str(raw_tsv), "--format", "tsv"])
        require(completed.returncode == 0, "raw replay should not require MIDI hardware")
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
