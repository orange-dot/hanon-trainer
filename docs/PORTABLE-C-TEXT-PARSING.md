# Portable C Text Parsing

## Purpose

Document the `strsep` portability issue found while adding macOS and Windows CI,
and record the parsing rule for TSV-like project metadata.

## Issue

`strsep` is not an ISO C function. It is a BSD/GNU extension that is commonly
visible on Linux/glibc when feature macros such as `_DEFAULT_SOURCE` are active.
That made the code look portable during Linux-only builds.

The cross-OS CI exposed the dependency:

- macOS failed in `src/content_catalog.c` with an implicit declaration for
  `strsep`.
- Windows/MSYS2 failed with the same implicit declaration.
- The compiler then treated the undeclared function as returning `int`, which
  produced an invalid assignment to `char*`.

The macOS failure also had a feature-macro trigger. The CMake configuration was
applying `_POSIX_C_SOURCE=200809L` to Unix-like targets. On macOS that POSIX-only
mode can hide Darwin/BSD extension declarations even when the libc implements
the function.

## Why Not `strtok`

`strtok` is not a correct replacement for this repository's TSV parsing.

For TSV data, empty fields are meaningful because they preserve the column
count. `strtok` skips empty fields and collapses repeated delimiters, so it can
silently change the parsed schema.

Example:

```text
a\t\tc
```

Expected TSV fields:

```text
["a", "", "c"]
```

`strtok` result:

```text
["a", "c"]
```

That behavior is unsafe for catalog and overlay metadata.

## Current Rule

Do not use `strsep` or `strtok` for project metadata parsing.

Use a local helper, or a shared internal parser if this pattern grows, that:

- uses only standard C functions
- modifies the caller-owned line buffer in place
- returns empty fields instead of skipping them
- advances an explicit cursor
- returns `NULL` only after the final field has been consumed

The current helper shape is:

```c
static char* split_next_field(char** cursor, char delimiter) {
    char* field;
    char* split;

    if ((cursor == NULL) || (*cursor == NULL)) {
        return NULL;
    }

    field = *cursor;
    split = strchr(field, delimiter);
    if (split == NULL) {
        *cursor = NULL;
    } else {
        *split = '\0';
        *cursor = split + 1;
    }
    return field;
}
```

This keeps TSV column accounting explicit and portable across Linux, macOS, and
Windows/MSYS2.

## Build-System Rule

Feature macros should be scoped by platform:

- Linux targets may use `_DEFAULT_SOURCE` and `_POSIX_C_SOURCE=200809L` where
  needed.
- macOS targets should not be forced into POSIX-only visibility; use
  `_DARWIN_C_SOURCE` when Darwin extension visibility is needed.
- Windows/MSYS2 should not depend on Unix extension declarations for core
  metadata parsing.

The portability target is C11 plus platform-specific APIs only inside explicit
platform boundary files.
