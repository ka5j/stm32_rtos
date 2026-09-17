#!/usr/bin/awk -f
#
# check_test_registration.awk
#
# Cross-checks every test function defined in tests/unit/*.c against the
# extern declarations and RUN_TEST() calls in tests/unit/test_runner.c.
#
# Unity requires exactly one main()/setUp()/tearDown() per binary, so
# test_runner.c owns them and reaches every other file's tests through a
# hand-maintained extern + RUN_TEST pair. Nothing else notices when one
# of those pairs is missing: the test file still compiles, the suite
# still passes, and the test simply never runs. `make coverage` would
# catch it for a driver test, but its filter is drivers/src only, so for
# the test_<peripheral>_reg.c files a forgotten registration is entirely
# silent. Same spirit as check_vector_table.awk: two hand-written lists
# with no shared source of truth, and nothing else watching them drift.
#
# Usage: awk -f tools/check_test_registration.awk tests/unit/*.c
#
# Written for POSIX awk (no gawk-only extensions - macOS ships the BWK
# "one true awk", not gawk, and this must run there too).

# ---- test_runner.c: collect extern declarations and RUN_TEST calls ----
FILENAME ~ /test_runner\.c$/ {
    if ($0 ~ /^extern void test_[A-Za-z0-9_]+\(void\);/) {
        name = $0
        sub(/^extern void /, "", name)
        sub(/\(void\);.*/, "", name)
        declared[name] = 1
    }
    if ($0 ~ /RUN_TEST\(test_[A-Za-z0-9_]+\)/) {
        name = $0
        sub(/^.*RUN_TEST\(/, "", name)
        sub(/\).*/, "", name)
        run[name] = 1
    }
    next
}

# ---- every other tests/unit/*.c: collect defined test functions ----
/^void test_[A-Za-z0-9_]+\(void\)/ {
    name = $0
    sub(/^void /, "", name)
    sub(/\(void\).*/, "", name)
    defined[name] = FILENAME
}

END {
    failures = 0

    for (name in defined) {
        if (!(name in declared)) {
            printf "MISSING extern:   %s (defined in %s)\n", name, defined[name]
            failures++
        }
        if (!(name in run)) {
            printf "MISSING RUN_TEST: %s (defined in %s)\n", name, defined[name]
            failures++
        }
    }

    for (name in run) {
        if (!(name in defined)) {
            printf "RUN_TEST for a test that no tests/unit/*.c defines: %s\n", name
            failures++
        }
    }

    if (failures > 0) {
        printf "\ncheck_test_registration: %d problem(s) - a test that is not\n", failures
        printf "registered in tests/unit/test_runner.c never runs.\n"
        exit 1
    }

    n = 0
    for (name in defined) { n++ }
    printf "OK: all %d tests in tests/unit/ are declared and run by test_runner.c\n", n
}
