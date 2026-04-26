#!/usr/bin/env python3
import argparse
import csv
import json
import pathlib
import subprocess
from collections import Counter


RAW_TSV_HEADER = ["ms", "raw"]
PROBE_TSV_HEADER = ["ms", "kind", "channel", "note", "velocity", "controller", "value", "raw_type"]
REQUIRED_TAKE = "2026-04-26-pc4-tui-002"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def read_meta(path: pathlib.Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8"))


def read_raw_tsv(path: pathlib.Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle, delimiter="\t")
        require(reader.fieldnames == RAW_TSV_HEADER, f"{path} has invalid raw TSV header")
        return list(reader)


def run_probe(probe: pathlib.Path, raw_tsv: pathlib.Path) -> list[dict[str, str]]:
    completed = subprocess.run(
        [str(probe), "--replay-raw-tsv", str(raw_tsv), "--format", "tsv"],
        text=True,
        capture_output=True,
        check=False,
    )
    require(completed.returncode == 0, f"{raw_tsv.name} replay failed: {completed.stderr}")
    rows = list(csv.DictReader(completed.stdout.splitlines(), delimiter="\t"))
    require(rows or completed.stdout.startswith("\t".join(PROBE_TSV_HEADER)),
            f"{raw_tsv.name} replay did not produce TSV")
    if rows:
        require(list(rows[0].keys()) == PROBE_TSV_HEADER, f"{raw_tsv.name} has invalid probe TSV header")
    return rows


def count_key(rows: list[dict[str, str]], key: str) -> dict[str, int]:
    return dict(sorted(Counter(row[key] for row in rows).items()))


def check_fixture(probe: pathlib.Path, raw_tsv: pathlib.Path, verbose: bool) -> dict[str, object]:
    take_id = raw_tsv.name.removesuffix(".raw.tsv")
    meta = read_meta(raw_tsv.with_name(f"{take_id}.meta.json"))
    raw_rows = read_raw_tsv(raw_tsv)
    probe_rows = run_probe(probe, raw_tsv)

    require(meta["source_take_id"] == take_id, f"{take_id} meta source_take_id mismatch")
    require(len(raw_rows) == int(meta["event_count"]), f"{take_id} raw event count mismatch")
    require(len(probe_rows) == int(meta["event_count"]), f"{take_id} replay event count mismatch")
    require(count_key(probe_rows, "kind") == meta["probe_kind_counts"],
            f"{take_id} replay kind counts differ from meta")
    require(count_key(probe_rows, "raw_type") == meta["raw_type_counts"],
            f"{take_id} replay raw_type counts differ from meta")

    if verbose:
        kinds = ", ".join(f"{key}={value}" for key, value in count_key(probe_rows, "kind").items())
        print(f"{take_id}: {len(probe_rows)} events, duration_ms={meta['duration_ms']}, {kinds}")
    return meta


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", required=True)
    parser.add_argument("--fixtures", required=True)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    fixtures = pathlib.Path(args.fixtures)
    raw_files = sorted(fixtures.glob("*.raw.tsv"))
    require(raw_files, "no PC4 live MIDI raw fixtures found")

    metas = [check_fixture(pathlib.Path(args.probe), raw_file, args.verbose) for raw_file in raw_files]
    take_ids = {str(meta["source_take_id"]) for meta in metas}
    require(REQUIRED_TAKE in take_ids, f"{REQUIRED_TAKE} fixture is required")

    total_events = sum(int(meta["event_count"]) for meta in metas)
    total_duration_ms = sum(int(meta["duration_ms"]) for meta in metas)
    aggregate_kinds: Counter[str] = Counter()
    for meta in metas:
        aggregate_kinds.update({key: int(value) for key, value in dict(meta["probe_kind_counts"]).items()})

    require(total_events >= 9000, "PC4 live MIDI fixtures should cover a large real event corpus")
    require(aggregate_kinds["note_on"] > 0, "PC4 live fixtures should include note_on")
    require(aggregate_kinds["note_off"] > 0, "PC4 live fixtures should include note_off")
    require(aggregate_kinds["controller"] > 0, "PC4 live fixtures should include controllers")
    require(aggregate_kinds["pitchbend"] > 0, "PC4 live fixtures should include pitchbend")
    require(aggregate_kinds["chanpress"] > 0, "PC4 live fixtures should include channel aftertouch")
    require(aggregate_kinds["pgmchange"] > 0, "PC4 live fixtures should include program changes")

    if args.verbose:
        print("== PC4 live MIDI fixture replay ==")
        print(f"fixtures: {len(raw_files)}")
        print(f"total_events: {total_events}")
        print(f"total_duration_ms: {total_duration_ms}")
        print("aggregate kinds:")
        for kind, count in sorted(aggregate_kinds.items()):
            print(f"  {kind}: {count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
