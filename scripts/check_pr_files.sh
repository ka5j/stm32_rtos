#!/bin/sh
# Checks on the files a pull request adds or modifies, run by CI on every
# pull_request and runnable locally with `make check-pr BASE=origin/develop`.
#
#   1. Every touched source file has a Doxygen @file block. `make docs` only
#      enforces per-member documentation on a file once it has at least one
#      Doxygen comment, so a file with none is otherwise invisible to it.
#   2. Every touched .c under a code layer (arch/, soc/, drivers/, board/)
#      has a test_<name>.c somewhere under tests/unit/, and every touched
#      *_reg.h is included by some file in tests/unit/. "Touched" means
#      added or modified.
#
# A new layer directory (the kernel, for example) is added to the patterns
# below, as well as to the Makefile's SRC_DIRS and INC_DIRS.
#
# Usage: scripts/check_pr_files.sh <base-ref>   e.g. origin/develop
set -u

if [ $# -ne 1 ]; then
    echo "usage: $0 <base-ref>" >&2
    exit 2
fi
base=$1
missing=0

touched() {
    git diff --name-status --diff-filter=AM "$base"...HEAD -- "$@" | awk '{print $2}'
}

# A pathspec's * also matches /, so 'drivers/*.c' covers drivers/src/.
for f in $(touched \
    'arch/*.h' 'arch/*.c' \
    'soc/*.h' 'soc/*.c' \
    'drivers/*.h' 'drivers/*.c' \
    'board/*.h' 'board/*.c' \
    'app/*.c'); do
    if ! grep -q '@file' "$f"; then
        echo "::error file=$f::Missing Doxygen @file block - make docs won't check this file at all without one"
        missing=1
    fi
done

# app/main.c is the entry point, not logic worth a dedicated unit test.
for f in $(touched 'arch/*.c' 'soc/*.c' 'drivers/*.c' 'board/*.c' 'app/*.c'); do
    [ "$f" = "app/main.c" ] && continue
    name=$(basename "$f" .c)
    if [ -z "$(find tests/unit -name "test_${name}.c")" ]; then
        echo "::error file=$f::New module $f has no matching test_${name}.c under tests/unit/"
        missing=1
    fi
done

for f in $(touched 'arch/*_reg.h' 'soc/*_reg.h'); do
    name=$(basename "$f")
    if ! grep -rl "\"$name\"" tests/unit/ >/dev/null 2>&1; then
        echo "::error file=$f::New register header $f is not included by any file in tests/unit/"
        missing=1
    fi
done

if [ "$missing" -eq 0 ]; then
    echo "OK: touched files have @file blocks and tests (against $base)"
fi
exit "$missing"
