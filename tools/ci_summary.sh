#!/usr/bin/env bash
#
# Writes CI's job summary: the release images' sizes (with the change
# against the pull request's base branch), the host test results and the
# coverage. Locally it prints the same Markdown to stdout.
#
# Environment (all optional):
#   GITHUB_STEP_SUMMARY  where to append the Markdown (default: stdout)
#   BASE_REF             branch to compare sizes against, e.g. develop
#   COVERAGE_LOG         saved output of `make coverage`
#
# Usage: tools/ci_summary.sh
set -uo pipefail

out=${GITHUB_STEP_SUMMARY:-/dev/stdout}
flash_bytes=$((512 * 1024)) # STM32F446RE (linker/STM32F446RE.ld)
ram_bytes=$((128 * 1024))
images=(stm32_rtos smoke)

# Prints "text data bss" of an ELF.
sections() {
    arm-none-eabi-size "$1" | awk 'NR == 2 { print $1, $2, $3 }'
}

# Builds the release images in directory $1; returns non-zero if either fails.
build_release() {
    make -C "$1" -s all BUILD=release >/dev/null 2>&1 &&
        make -C "$1" -s smoke BUILD=release >/dev/null 2>&1
}

# Formats a signed difference, e.g. "+12" or "-4" or "0".
delta() {
    if [ "$1" -gt 0 ]; then printf '+%d' "$1"; else printf '%d' "$1"; fi
}

{
    echo "## Release image size"
    echo
    built=no
    if build_release .; then
        built=yes
        echo "| Image | text | data | bss | Flash (text+data) | RAM (data+bss) |"
        echo "| ----- | ---: | ---: | --: | ----------------: | -------------: |"
        for image in "${images[@]}"; do
            read -r text data bss <<<"$(sections "build/$image.elf")"
            awk -v n="$image" -v t="$text" -v d="$data" -v b="$bss" \
                -v f="$flash_bytes" -v r="$ram_bytes" 'BEGIN {
                    printf "| %s | %d | %d | %d | %d (%.2f%%) | %d (%.2f%%) |\n",
                        n, t, d, b, t + d, 100 * (t + d) / f, d + b, 100 * (d + b) / r
                }'
        done
    else
        echo "The release build failed; see the build steps."
    fi

    if [ -n "${BASE_REF:-}" ]; then
        echo
        echo "### Change against \`$BASE_REF\`"
        echo
        base=$(mktemp -d)
        if [ "$built" = yes ] && git fetch -q origin "$BASE_REF" &&
            git worktree add -q --detach "$base/tree" "origin/$BASE_REF" &&
            build_release "$base/tree"; then
            echo "| Image | text | data | bss |"
            echo "| ----- | ---: | ---: | --: |"
            for image in "${images[@]}"; do
                read -r t1 d1 b1 <<<"$(sections "build/$image.elf")"
                read -r t0 d0 b0 <<<"$(sections "$base/tree/build/$image.elf")"
                echo "| $image | $(delta $((t1 - t0))) | $(delta $((d1 - d0))) | $(delta $((b1 - b0))) |"
            done
        else
            echo "n/a: the base branch or this branch could not be built for comparison."
        fi
        git worktree remove --force "$base/tree" >/dev/null 2>&1
        rm -rf "$base"
    fi

    echo
    echo "## Host tests"
    echo
    if [ -x tests/build/run_tests ]; then
        result=$(tests/build/run_tests 2>&1 | grep -E '^[0-9]+ Tests ' | tail -n 1)
        echo "${result:-No result line; see the test steps.}"
    else
        echo "Not built; see the test steps."
    fi

    echo
    echo "## Coverage (drivers/, bsp/)"
    echo
    if [ -n "${COVERAGE_LOG:-}" ] && grep -qE '^(lines|branches):' "$COVERAGE_LOG" 2>/dev/null; then
        grep -E '^(lines|branches):' "$COVERAGE_LOG" | sed 's/^/- /'
    else
        echo "Not available; see the coverage step."
    fi
} >>"$out"
