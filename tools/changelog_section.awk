#!/usr/bin/awk -f
#
# changelog_section.awk
#
# Prints the body of one release's section of CHANGELOG.md - everything
# between its "## [X.Y.Z]" heading and the next "## [" heading, without the
# heading itself. Used by the release workflow to turn the changelog entry
# into the GitHub Release notes, so the notes cannot drift from the
# changelog. Exits 1, printing nothing useful, if the version has no
# heading or an empty section, so a release is never published with blank
# notes.
#
# Usage: awk -v version=X.Y.Z -f tools/changelog_section.awk CHANGELOG.md
#
# Written for POSIX awk (no gawk-only extensions).

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
