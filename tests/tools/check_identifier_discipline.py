#!/usr/bin/env python3
import argparse
import pathlib
import re


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*", "", text)
    return text


def is_reserved_c_or_compiler_token(token: str) -> bool:
    return token.startswith("__") or (len(token) > 1 and token[1].isupper())


def find_typedef_names(text: str) -> list[str]:
    names: list[str] = []
    for match in re.finditer(r"typedef\s+struct\s+\w+\s+(\w+)\s*;", text):
        names.append(match.group(1))
    for match in re.finditer(r"}\s*(\w+)\s*;", text):
        names.append(match.group(1))
    return names


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--include", required=True)
    args = parser.parse_args()

    include_root = pathlib.Path(args.include)
    failures: list[str] = []
    for header in sorted(include_root.glob("hanon_trainer/*.h")):
        clean = strip_comments(header.read_text(encoding="utf-8"))
        for name in find_typedef_names(clean):
            if name.endswith("_t"):
                failures.append(f"{header}: typedef '{name}' uses reserved _t suffix")
        for token in re.findall(r"\b_[A-Za-z_]\w*\b", clean):
            if not is_reserved_c_or_compiler_token(token):
                failures.append(f"{header}: token '{token}' uses leading underscore")

    if failures:
        for failure in failures:
            print(failure)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
