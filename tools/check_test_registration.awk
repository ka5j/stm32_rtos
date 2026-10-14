#!/usr/bin/awk -f
#
# Checks that every test function in tests/unit/*.c is declared extern and
# RUN_TEST()'d in test_runner.c. A missing pair still compiles and passes,
# and the test silently never runs; coverage would not notice for the
# register tests. Run by `make test`. POSIX awk (macOS ships BWK awk).
#
# Usage: awk -f tools/check_test_registration.awk tests/unit/*.c

# ---- comments and #if 0 are not code ----
# Every rule below looks at the line with comments and `#if 0` regions
# removed. Without that, a RUN_TEST(...) inside a /* ... */ block (or inside
# `#if 0`) still looked like a live registration - the "test compiles, never
# runs" failure this script exists to catch - and a test defined inside one
# counted as defined. Line comments, block comments (one line or many) and
# `#if 0 ... #endif` (with `#else`/`#elif` ending the region) are handled;
# string literals are not parsed, which is fine for test code.
function strip(line,    out, s, i, j) {
    out = ""
    s = line
    while (s != "") {
        if (in_block) {
            i = index(s, "*/")
            if (i == 0) { return out }
            s = substr(s, i + 2)
            in_block = 0
        } else {
            i = index(s, "/*")
            j = index(s, "//")
            if (j > 0 && (i == 0 || j < i)) { return out substr(s, 1, j - 1) }
            if (i == 0) { return out s }
            out = out substr(s, 1, i - 1)
            s = substr(s, i + 2)
            in_block = 1
        }
    }
    return out
}

function live_code(line,    code) {
    code = strip(line)
    if (disabled > 0) {
        if (code ~ /^[ \t]*#[ \t]*if/) { disabled++ }
        else if (code ~ /^[ \t]*#[ \t]*endif/) { disabled-- }
        else if (disabled == 1 && code ~ /^[ \t]*#[ \t]*(else|elif)/) { disabled = 0 }
        return ""
    }
    if (code ~ /^[ \t]*#[ \t]*if[ \t]+0[ \t]*$/) { disabled = 1; return "" }
    return code
}

FNR == 1 { in_block = 0; disabled = 0 }
{ $0 = live_code($0) }

# ---- test_runner.c: collect extern declarations and RUN_TEST calls ----
FILENAME ~ /test_runner\.c$/ {
    if ($0 ~ /^extern void test_[A-Za-z0-9_]+\(void\);/) {
        name = $0
        sub(/^extern void /, "", name)
        sub(/\(void\);.*/, "", name)
        declared[name] = 1
    }
    # Only a live statement counts as a registration: a commented-out or
    # `#if 0`'d RUN_TEST(test_x); is already gone from $0 (see above), so
    # it cannot satisfy the check while the test silently never runs.
    if ($0 ~ /^[ \t]*RUN_TEST\(test_[A-Za-z0-9_]+\);/) {
        name = $0
        sub(/^[ \t]*RUN_TEST\(/, "", name)
        sub(/\).*/, "", name)
        if (name in run) {
            printf "DUPLICATE RUN_TEST: %s is registered twice\n", name
            duplicates++
        }
        run[name] = 1
    } else if ($0 ~ /RUN_TEST\(test_[A-Za-z0-9_]+\)/) {
        printf "RUN_TEST that is not a live statement (commented out?): %s\n", $0
        duplicates++
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

# A test function that does not match the strict `void test_x(void)` form
# above would be invisible to this check: flag any other column-0 definition
# whose name starts with test_.
/^[A-Za-z_][A-Za-z0-9_ \*]* test_[A-Za-z0-9_]+\(/ && $0 !~ /^void test_[A-Za-z0-9_]+\(void\)/ && $0 !~ /^(static|extern) / && $0 !~ /;/ {
    printf "UNRECOGNISED test signature (must be `void test_x(void)`): %s\n", $0
    odd++
}

# A definition split as `void` on one line and `test_x(void)` on the next
# starts with the name at column 0 - invisible to both patterns above, so
# the test would never be counted. Calls to a test function are indented
# inside other functions, so a column-0 `test_x(` is always a definition.
/^test_[A-Za-z0-9_]+\(/ && $0 !~ /;/ {
    printf "UNRECOGNISED test signature (return type and name must share a line): %s\n", $0
    odd++
}

END {
    failures = duplicates + odd

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
    if (n == 0) {
        print "check_test_registration: found no tests - parsing broke"
        exit 1
    }
    printf "OK: all %d tests in tests/unit/ are declared and run by test_runner.c\n", n
}
