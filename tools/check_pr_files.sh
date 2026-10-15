#!/bin/sh
# Checks on the files a pull request adds or modifies, run by CI on every
# pull_request and runnable locally with `make check-pr BASE=origin/develop`.
#
#   1. Every touched source file has a Doxygen @file block. `make docs` only
#      enforces per-member documentation on a file once it has at least one
#      Doxygen comment, so a file with none is otherwise invisible to it.
#   2. Every touched .c under drivers/, api/, bsp/ or rtos/ has a
#      tests/unit/test_<name>.c, and every touched *_reg.h is included by
#      some file in tests/unit/. "Touched" means added or modified.
#
# Usage: tools/check_pr_files.sh <base-ref>   e.g. origin/develop
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

for f in $(touched \
    'core/inc/*.h' 'device/inc/*.h' \
    'drivers/inc/*.h' 'drivers/src/*.c' \
    'api/inc/*.h' 'api/src/*.c' \
    'bsp/inc/*.h' 'bsp/src/*.c' \
    'rtos/kernel/inc/*.h' 'rtos/kernel/src/*.c' \
    'rtos/api/inc/*.h' 'rtos/api/src/*.c' \
    'app/src/*.c'); do
    if ! grep -q '@file' "$f"; then
        echo "::error file=$f::Missing Doxygen @file block - make docs won't check this file at all without one"
        missing=1
    fi
done

# app/src/main.c is the entry point, not logic worth a dedicated unit test.
for f in $(touched 'drivers/*.c' 'api/*.c' 'bsp/*.c' 'rtos/*.c' 'app/*.c'); do
    [ "$f" = "app/src/main.c" ] && continue
    name=$(basename "$f" .c)
    if [ ! -f "tests/unit/test_${name}.c" ]; then
        echo "::error file=$f::New module $f has no matching tests/unit/test_${name}.c"
        missing=1
    fi
done

for f in $(touched 'core/inc/*_reg.h' 'device/inc/*_reg.h'); do
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
