#!/usr/bin/env bash
# Prints the next semantic version (vX.Y.Z) based on commits since the last tag.
#
#   BREAKING CHANGE in body, or "type!:" in subject  -> major
#   "feat:" / "feat(scope):"                          -> minor
#   anything else                                     -> patch
#
# While the project is below 1.0.0, a breaking change bumps the minor
# version instead of jumping straight to 1.0.0.
set -euo pipefail

last_tag=$(git describe --tags --abbrev=0 --match 'v[0-9]*.[0-9]*.[0-9]*' 2>/dev/null || true)

if [[ -z "$last_tag" ]]; then
    echo "v0.1.0"
    exit 0
fi

range="$last_tag..HEAD"
if [[ -z "$(git rev-list "$range")" ]]; then
    echo "$last_tag"
    exit 0
fi

IFS=. read -r major minor patch <<< "${last_tag#v}"

subjects=$(git log --format='%s' "$range")
bodies=$(git log --format='%b' "$range")

if grep -qE '^[a-z]+(\([^)]*\))?!:' <<< "$subjects" || grep -q 'BREAKING CHANGE' <<< "$bodies"; then
    if (( major == 0 )); then
        minor=$((minor + 1)); patch=0
    else
        major=$((major + 1)); minor=0; patch=0
    fi
elif grep -qE '^feat(\([^)]*\))?:' <<< "$subjects"; then
    minor=$((minor + 1)); patch=0
else
    patch=$((patch + 1))
fi

echo "v$major.$minor.$patch"
