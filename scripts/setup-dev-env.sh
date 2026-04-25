#!/usr/bin/env sh
set -eu

usage() {
  cat <<'USAGE'
Usage: scripts/setup-dev-env.sh [--verify-only]

Install and verify the native development packages needed for hanon-trainer:
C compiler, CMake, Ninja, pkg-config, SQLite development files, SDL2,
SDL2_ttf, Poppler tools, ALSA development files, and ALSA MIDI utilities.

Options:
  --verify-only    Do not install packages; only run environment checks.
USAGE
}

VERIFY_ONLY=0
case "${1:-}" in
  "")
    ;;
  --verify-only)
    VERIFY_ONLY=1
    ;;
  -h|--help)
    usage
    exit 0
    ;;
  *)
    usage >&2
    exit 2
    ;;
esac

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "missing command: $1" >&2
    return 1
  fi
}

sudo_cmd() {
  if [ "$(id -u)" -eq 0 ]; then
    "$@"
  else
    sudo "$@"
  fi
}

install_packages() {
  if [ "$VERIFY_ONLY" -eq 1 ]; then
    return 0
  fi

  if command -v dnf >/dev/null 2>&1; then
    sudo_cmd dnf install -y \
      gcc \
      make \
      cmake \
      ninja-build \
      pkgconf-pkg-config \
      sqlite \
      sqlite-devel \
      SDL2-devel \
      SDL2_ttf-devel \
      poppler-utils \
      alsa-lib-devel \
      alsa-utils \
      git \
      which
    return 0
  fi

  if command -v apt-get >/dev/null 2>&1; then
    sudo_cmd apt-get update
    sudo_cmd apt-get install -y \
      build-essential \
      cmake \
      ninja-build \
      pkg-config \
      sqlite3 \
      libsqlite3-dev \
      libsdl2-dev \
      libsdl2-ttf-dev \
      poppler-utils \
      libasound2-dev \
      alsa-utils \
      git
    return 0
  fi

  if command -v pacman >/dev/null 2>&1; then
    sudo_cmd pacman -S --needed \
      base-devel \
      cmake \
      ninja \
      pkgconf \
      sqlite \
      sdl2 \
      sdl2_ttf \
      poppler \
      alsa-lib \
      alsa-utils \
      git
    return 0
  fi

  if command -v zypper >/dev/null 2>&1; then
    sudo_cmd zypper install -y \
      gcc \
      make \
      cmake \
      ninja \
      pkgconf-pkg-config \
      sqlite3 \
      sqlite3-devel \
      libSDL2-devel \
      libSDL2_ttf-devel \
      poppler-tools \
      alsa-devel \
      alsa-utils \
      git \
      which
    return 0
  fi

  cat >&2 <<'ERROR'
Unsupported package manager.

Install these packages manually:
  C compiler, make, cmake, ninja, pkg-config,
  sqlite3 + sqlite3 development headers,
  SDL2 development headers,
  SDL2_ttf development headers,
  poppler-utils / pdftoppm,
  ALSA development headers,
  ALSA MIDI utilities / aconnect / aseqdump,
  git.
ERROR
  exit 1
}

verify_pkg_config() {
  pkg="$1"
  if ! pkg-config --exists "$pkg"; then
    echo "pkg-config cannot find: $pkg" >&2
    return 1
  fi
  printf '%s ' "$pkg"
  pkg-config --modversion "$pkg"
}

compile_probe() {
  name="$1"
  cflags="$2"
  libs="$3"
  source="$4"
  tmpdir="${TMPDIR:-/tmp}/hanon-trainer-dev-probe"
  mkdir -p "$tmpdir"
  src="$tmpdir/$name.c"
  bin="$tmpdir/$name"
  printf '%s\n' "$source" > "$src"
  # shellcheck disable=SC2086
  cc -std=c11 -Wall -Wextra -Wpedantic $cflags "$src" $libs -o "$bin"
}

verify_environment() {
  failures=0

  echo "== commands =="
  for cmd in cc cmake ninja pkg-config pdftoppm sqlite3 aconnect aseqdump; do
    if need_cmd "$cmd"; then
      echo "$cmd: ok"
    else
      failures=1
    fi
  done

  echo "== versions =="
  command -v cc >/dev/null 2>&1 && cc --version | sed -n '1p'
  command -v cmake >/dev/null 2>&1 && cmake --version | sed -n '1p'
  command -v ninja >/dev/null 2>&1 && ninja --version
  command -v pdftoppm >/dev/null 2>&1 && pdftoppm -v 2>&1 | sed -n '1p'
  command -v sqlite3 >/dev/null 2>&1 && sqlite3 --version | sed -n '1p'
  command -v aconnect >/dev/null 2>&1 && printf 'aconnect: %s\n' "$(command -v aconnect)"
  command -v aseqdump >/dev/null 2>&1 && printf 'aseqdump: %s\n' "$(command -v aseqdump)"

  echo "== pkg-config =="
  if command -v pkg-config >/dev/null 2>&1; then
    for pkg in sqlite3 sdl2 SDL2_ttf alsa; do
      if ! verify_pkg_config "$pkg"; then
        failures=1
      fi
    done
  else
    failures=1
  fi

  if [ "$failures" -ne 0 ]; then
    echo "environment verification failed before compile probes" >&2
    return 1
  fi

  echo "== compile probes =="
  sqlite_cflags="$(pkg-config --cflags sqlite3)"
  sqlite_libs="$(pkg-config --libs sqlite3)"
  sdl_cflags="$(pkg-config --cflags sdl2 SDL2_ttf)"
  sdl_libs="$(pkg-config --libs sdl2 SDL2_ttf)"
  alsa_cflags="$(pkg-config --cflags alsa)"
  alsa_libs="$(pkg-config --libs alsa)"

  compile_probe sqlite "$sqlite_cflags" "$sqlite_libs" \
    '#include <sqlite3.h>
int main(void) {
  sqlite3* db = 0;
  return sqlite3_open(":memory:", &db) == SQLITE_OK ? sqlite3_close(db) : 1;
}'
  echo "sqlite compile probe: ok"

  compile_probe sdl "$sdl_cflags" "$sdl_libs" \
    '#include <SDL.h>
#include <SDL_ttf.h>
int main(void) {
  return SDL_VERSION_ATLEAST(2, 0, 0) && TTF_MAJOR_VERSION >= 2 ? 0 : 1;
}'
  echo "SDL2/SDL2_ttf compile probe: ok"

  compile_probe alsa "$alsa_cflags" "$alsa_libs" \
    '#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <alsa/asoundlib.h>
int main(void) {
  return SND_SEQ_OPEN_INPUT >= 0 ? 0 : 1;
}'
  echo "ALSA compile probe: ok"
}

install_packages
verify_environment

echo "hanon-trainer dev environment: ok"
