#!/usr/bin/awk -f
#
# check_version.awk
#
# Cross-checks the three places a release version is recorded: the Doxyfile's
# PROJECT_NUMBER, the newest released heading in CHANGELOG.md (the first
# "## [X.Y.Z]" below "## [Unreleased]"), and - when given - the git tag.
# docs/VERSIONING.md requires all three to agree at a release and nothing
# else watches them: they are edited by hand in different files, so a missed
# one ships a release whose generated docs or changelog name the wrong
# version.
#
# Between releases the check still holds, because PROJECT_NUMBER names the
# last version that shipped and "[Unreleased]" is skipped, so it is safe to
# run on every PR.
#
# Usage: awk -v tag=vX.Y.Z -f tools/check_version.awk Doxyfile CHANGELOG.md
#        (omit -v tag=... to compare only the Doxyfile and CHANGELOG)
#
# Written for POSIX awk (no gawk-only extensions - macOS ships the BWK
# "one true awk", not gawk, and this must run there too).

FILENAME ~ /Doxyfile$/ && $1 == "PROJECT_NUMBER" && $2 == "=" {
    doxyfile = $3
}

FILENAME ~ /CHANGELOG\.md$/ && changelog == "" && /^## \[[0-9]+\.[0-9]+\.[0-9]+\]/ {
    changelog = $2
    sub(/^\[/, "", changelog)
    sub(/\]$/, "", changelog)
}

END {
    fail = 0

    if (doxyfile == "") {
        print "check_version: no PROJECT_NUMBER found in Doxyfile" > "/dev/stderr"
        fail = 1
    }
    if (changelog == "") {
        print "check_version: no released \"## [X.Y.Z]\" heading found in CHANGELOG.md" > "/dev/stderr"
        fail = 1
    }
    if (doxyfile != "" && changelog != "" && doxyfile != changelog) {
        printf "check_version: Doxyfile PROJECT_NUMBER is %s but CHANGELOG.md's newest release is %s\n", doxyfile, changelog > "/dev/stderr"
        fail = 1
    }
    if (tag != "") {
        if (tag !~ /^v[0-9]+\.[0-9]+\.[0-9]+$/) {
            printf "check_version: tag \"%s\" is not of the form vX.Y.Z\n", tag > "/dev/stderr"
            fail = 1
        } else if (substr(tag, 2) != doxyfile) {
            printf "check_version: tag %s does not match Doxyfile PROJECT_NUMBER %s\n", tag, doxyfile > "/dev/stderr"
            fail = 1
        }
    }

    if (fail == 0) {
        printf "OK: Doxyfile, CHANGELOG.md%s all name version %s\n", (tag != "" ? " and tag" : ""), doxyfile
    }
    exit fail
}
