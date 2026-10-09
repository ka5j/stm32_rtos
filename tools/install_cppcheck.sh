#!/bin/sh
#
# install_cppcheck.sh
#
# Builds and installs the exact cppcheck version CI uses, into a per-user
# prefix (no sudo, nothing outside $PREFIX is touched). `make lint` picks it
# up automatically once it is there.
#
# Why this exists: different cppcheck versions report different MISRA
# findings. CI pins 2.13.0 (.github/workflows/ci.yml); Homebrew and most
# distros ship something newer, and a newer cppcheck can pass code that
# 2.13.0 rejects - a `cond ? A : B` expression passed locally under 2.21 and
# failed CI under 2.13.0 (misra-c2012-10.6), costing a CI cycle. Homebrew
# has no formula for an older cppcheck, so it is built from source.
#
# Usage: tools/install_cppcheck.sh [PREFIX]
#        PREFIX defaults to $HOME/.local/share/cppcheck-2.13.0
#
# Needs: git, a C++ compiler, make, python3 (cppcheck's MISRA addon is a
# Python script). Takes a few minutes. When CI's pin changes, update
# VERSION and COMMIT here and CPPCHECK_CI_VERSION in the Makefile.

set -eu

VERSION=2.13.0
# The commit the 2.13.0 tag points at. Checked after cloning so a moved or
# tampered tag fails here instead of being built.
COMMIT=da29903ffcbde465b6c2b47e8dc38277743f47ec
PREFIX=${1:-$HOME/.local/share/cppcheck-$VERSION}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

echo "Cloning cppcheck $VERSION..."
git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$VERSION" https://github.com/danmar/cppcheck.git "$tmp/src"

actual=$(git -C "$tmp/src" rev-parse HEAD)
if [ "$actual" != "$COMMIT" ]; then
    echo "install_cppcheck: tag $VERSION resolves to $actual, expected $COMMIT" >&2
    exit 1
fi

jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
echo "Building with $jobs jobs (a few minutes)..."
# FILESDIR is baked in so the binary finds its cfg/ and addons/ under
# $PREFIX; MATCHCOMPILER speeds up the analysis.
# cppcheck's own source produces pages of compiler warnings; keep them in a
# log and show only its tail if the build actually fails.
if ! make -C "$tmp/src" -j"$jobs" MATCHCOMPILER=yes FILESDIR="$PREFIX" CXXFLAGS="-O2 -DNDEBUG" cppcheck >"$tmp/build.log" 2>&1; then
    echo "install_cppcheck: build failed; last lines of the build log:" >&2
    tail -n 30 "$tmp/build.log" >&2
    exit 1
fi

mkdir -p "$PREFIX/bin"
cp "$tmp/src/cppcheck" "$PREFIX/bin/"
cp -R "$tmp/src/cfg" "$tmp/src/addons" "$tmp/src/platforms" "$PREFIX/"

echo "Installed: $("$PREFIX/bin/cppcheck" --version) at $PREFIX/bin/cppcheck"
echo "make lint now uses it automatically (override with CPPCHECK=...)."
