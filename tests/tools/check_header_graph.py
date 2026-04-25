#!/usr/bin/env python3
import argparse
import pathlib
import re


ALLOWED = {
    "ht_types.h": set(),
    "content_catalog.h": {"ht_types.h"},
    "overlay_store.h": {"ht_types.h"},
    "session_core.h": {"ht_types.h"},
    "midi_capture.h": {"ht_types.h"},
    "sqlite_store.h": {"ht_types.h"},
    "performance_analysis.h": {"content_catalog.h", "overlay_store.h", "sqlite_store.h"},
    "codex_cli_adapter.h": {"ht_types.h"},
    "advice_orchestrator.h": {"codex_cli_adapter.h", "content_catalog.h", "sqlite_store.h"},
    "score_renderer.h": {"ht_types.h"},
    "guidance_renderer.h": {"ht_types.h"},
}


def project_includes(text: str) -> set[str]:
    found: set[str] = set()
    for match in re.finditer(r"#\s*include\s+[<\"]hanon_trainer/([^>\"]+)[>\"]", text):
        found.add(pathlib.Path(match.group(1)).name)
    return found


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--include", required=True)
    args = parser.parse_args()

    include_root = pathlib.Path(args.include) / "hanon_trainer"
    failures: list[str] = []
    for header_name, allowed in sorted(ALLOWED.items()):
        path = include_root / header_name
        actual = project_includes(path.read_text(encoding="utf-8"))
        unexpected = actual - allowed
        if unexpected:
            failures.append(f"{header_name}: unexpected project includes {sorted(unexpected)}")
    for path in sorted(include_root.glob("*.h")):
        if path.name not in ALLOWED:
            failures.append(f"{path.name}: missing from documented header graph")

    if failures:
        for failure in failures:
            print(failure)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
