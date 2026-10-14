#!/usr/bin/awk -f
#
# Checks IRQn_e in core/inc/nvic_reg.h against the vector table in
# startup/startup_stm32f446re.s, position by position: the 16 core words, the
# negative IRQn_e entries (so a swapped SVC/PendSV/SysTick slot fails), and
# all 97 peripheral slots. A wrong IRQn would silently route an NVIC call to
# the wrong line. Run by `make test`. POSIX awk.
#
# Usage: awk -f tools/check_vector_table.awk core/inc/nvic_reg.h \
#              startup/startup_stm32f446re.s

BEGIN {
    in_enum = 0
    in_vec = 0
    idx = -1
    CORE_WORDS = 16 # ARMv7-M: initial SP + 15 core exception vectors,
                     # fixed by the architecture, before any peripheral
                     # IRQ starts at word index 16 / IRQn 0.
    EXPECTED_PERIPHERAL_SLOTS = 97 # the F446 has IRQ 0..96; a truncated or
                                   # padded table must not pass just because
                                   # the two files agree with each other.
    # ARMv7-M core vector words, in order (a literal 0 is a reserved slot).
    core[0] = "_estack";           core[1] = "Reset_Handler"
    core[2] = "NMI_Handler";       core[3] = "HardFault_Handler"
    core[4] = "MemManage_Handler"; core[5] = "BusFault_Handler"
    core[6] = "UsageFault_Handler"
    core[7] = "0"; core[8] = "0"; core[9] = "0"; core[10] = "0"
    core[11] = "SVC_Handler";      core[12] = "DebugMon_Handler"
    core[13] = "0";                core[14] = "PendSV_Handler"
    core[15] = "SysTick_Handler"
    # IRQn_e names that differ from their vector's handler name.
    handlerFor["SVCall"] = "SVC_Handler"
    handlerFor["MemoryManagement"] = "MemManage_Handler"
}

# ---- File 1: core/inc/nvic_reg.h - extract IRQn_e name/value pairs ----
NR == FNR {
    if ($0 ~ /typedef enum IRQn_e/) { in_enum = 1; next }
    if (in_enum && $0 ~ /IRQn_e;/) { in_enum = 0; next }
    if (in_enum && $0 ~ /_IRQn = -?[0-9]+,/) {
        line = $0
        sub(/,.*/, "", line)
        sub(/^[ \t]+/, "", line)
        split(line, parts, " = ")
        name = parts[1]
        sub(/_IRQn$/, "", name)
        val = parts[2] + 0
        if (val < 0) {
            sysName[val] = name
        } else {
            if (val in enumName) {
                printf "ERROR: nvic_reg.h defines IRQn %d twice (%s and %s)\n", val, enumName[val], name
                enumDupes++
            }
            enumName[val] = name
            enumCount++
        }
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
    errors = enumDupes + 0

    if (peripheral_count != EXPECTED_PERIPHERAL_SLOTS) {
        printf "ERROR: the vector table has %d peripheral slots, the F446 has %d (IRQ 0..%d)\n", \
            peripheral_count, EXPECTED_PERIPHERAL_SLOTS, EXPECTED_PERIPHERAL_SLOTS - 1
        errors++
    }

    # The architecturally fixed core words.
    for (c = 0; c < CORE_WORDS; c++) {
        if (word[c] != core[c]) {
            printf "ERROR: core vector word %d is %s, ARMv7-M requires %s\n", c, word[c], core[c]
            errors++
        }
    }

    # Every negative IRQn_e entry must name the core word it claims.
    for (v in sysName) {
        expected = (sysName[v] in handlerFor) ? handlerFor[sysName[v]] : sysName[v] "_Handler"
        w = word[CORE_WORDS + v]
        if (w != expected) {
            printf "ERROR: IRQn_e %s_IRQn = %d implies vector word %d = %s, but it is %s\n", \
                sysName[v], v, CORE_WORDS + v, expected, w
            errors++
        }
    }

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
