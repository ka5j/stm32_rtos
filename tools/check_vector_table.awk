#!/usr/bin/awk -f
#
# check_vector_table.awk
#
# Cross-checks core/inc/nvic_reg.h's IRQn_e enum against
# startup/startup_stm32f446re.s's vector table, position by position.
# Catches the two silently drifting apart - a wrong IRQn value would
# route a driver's NVIC enable/priority call to the wrong interrupt
# line without a compiler or linker ever noticing.
#
# Usage: awk -f tools/check_vector_table.awk core/inc/nvic_reg.h \
#              startup/startup_stm32f446re.s
#
# Written for POSIX awk (no gawk-only extensions - macOS ships the BWK
# "one true awk", not gawk, and this must run there too).

BEGIN {
    in_enum = 0
    in_vec = 0
    idx = -1
    CORE_WORDS = 16 # ARMv7-M: initial SP + 15 core exception vectors,
                     # fixed by the architecture, before any peripheral
                     # IRQ starts at word index 16 / IRQn 0.
}

# ---- File 1: core/inc/nvic_reg.h - extract IRQn_e name/value pairs ----
NR == FNR {
    if ($0 ~ /typedef enum IRQn_e/) { in_enum = 1; next }
    if (in_enum && $0 ~ /IRQn_e;/) { in_enum = 0; next }
    if (in_enum && $0 ~ /_IRQn = [0-9]+,/) {
        line = $0
        sub(/,.*/, "", line)
        sub(/^[ \t]+/, "", line)
        split(line, parts, " = ")
        name = parts[1]
        sub(/_IRQn$/, "", name)
        val = parts[2] + 0
        enumName[val] = name
        enumCount++
    }
    next
}

# ---- File 2: startup_stm32f446re.s - extract vector table word order ----
{
    if ($0 == "vector_table:") { in_vec = 1; idx = 0; next }
    if (in_vec && $0 ~ /Weak aliases/) { in_vec = 0; next }
    if (in_vec && $1 == ".word") {
        word[idx] = $2
        idx++
    }
}

END {
    if (enumCount == 0) {
        print "ERROR: found no IRQn_e entries in core/inc/nvic_reg.h - parsing broke"
        exit 1
    }
    if (idx <= CORE_WORDS) {
        print "ERROR: found no vector table entries in startup_stm32f446re.s - parsing broke"
        exit 1
    }

    peripheral_count = idx - CORE_WORDS
    errors = 0

    for (p = 0; p < peripheral_count; p++) {
        w = word[CORE_WORDS + p]
        has_enum = (p in enumName)
        if (w == "0") {
            if (has_enum) {
                printf "ERROR: vector table position %d is reserved (0) but nvic_reg.h defines %s_IRQn = %d\n", \
                    p, enumName[p], p
                errors++
            }
        } else if (!has_enum) {
            printf "ERROR: vector table position %d defines %s but nvic_reg.h has no IRQn_e entry with value %d\n", \
                p, w, p
            errors++
        } else {
            expected = enumName[p] "_IRQHandler"
            if (w != expected) {
                printf "ERROR: position %d: vector table has %s, nvic_reg.h's IRQn_e implies %s\n", \
                    p, w, expected
                errors++
            }
        }
    }

    # Catch an enum value with no corresponding word at all (out of range).
    for (v in enumName) {
        if (v + 0 >= peripheral_count) {
            printf "ERROR: nvic_reg.h defines %s_IRQn = %d, past the end of the vector table (%d peripheral slots)\n", \
                enumName[v], v, peripheral_count
            errors++
        }
    }

    if (errors == 0) {
        printf "OK: IRQn_e (core/inc/nvic_reg.h) matches startup_stm32f446re.s vector table (%d peripheral positions checked)\n", \
            peripheral_count
        exit 0
    }
    printf "%d mismatch(es) found between nvic_reg.h and startup_stm32f446re.s\n", errors
    exit 1
}
