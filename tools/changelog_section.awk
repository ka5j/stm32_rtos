#!/usr/bin/awk -f
#
# Prints one release's CHANGELOG.md section body (without its heading), for
# the GitHub Release notes. Exits 1 if the section is missing or empty, so a
# release is never published with blank notes. POSIX awk.
#
# Usage: awk -v version=X.Y.Z -f tools/changelog_section.awk CHANGELOG.md

/^## \[/ {
    in_section = (index($0, "## [" version "]") == 1)
    next
}

in_section {
    print
    if ($0 !~ /^[ \t]*$/) {
        has_content = 1
    }
}

END {
    if (!has_content) {
        printf "changelog_section: no non-empty section for version %s in CHANGELOG.md\n", version > "/dev/stderr"
        exit 1
    }
}
