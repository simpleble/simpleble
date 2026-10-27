#!/bin/bash

validate_version_format() {
    local version_file_content=$1
    echo "- VERSION file format? ($version_file_content)"
    if [[ ! "$version_file_content" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-dev)?$ ]]; then
        echo "  FAIL, format should be major.minor.patch, optionally followed by -dev"
        return 1
    else
        echo "  OK"
        return 0
    fi
}

# Function to check that VERSION names a release after the last tag, so local builds never carry a released version
check_version_after_last_tag() {
    local version_file=$1
    local last_tag=$2
    local version_base=${version_file%-dev}
    local smallest=$(printf '%s\n' "$version_base" "$last_tag" | sort -V | head -n1)
    echo "- VERSION file ($version_file) after last released tag ($last_tag)?"
    if [[ "$version_base" == "$last_tag" || "$smallest" == "$version_base" ]]; then
        echo "  FAIL VERSION should name the next release, did you forget to bump it after the release?"
        return 1
    else
        echo "  OK"
        return 0
    fi
}

# Function to check if Cargo.toml version matches the VERSION file
check_cargo_version_match() {
    local version_file=$1
    # Extract the version line from Cargo.toml
    local cargo_version=$2
    echo "- Cargo.toml consistent with VERSION file?"
    if [[ "$cargo_version" == "$version_file" ]]; then
        echo "  OK"
        return 0
    else
        echo "  FAIL Cargo.toml version ($cargo_version) does not match VERSION file content ($version_file)."
        return 1
    fi
}

# Function to check if simpleaible/pyproject.toml version matches the VERSION file
check_pyproject_version_match() {
    local version_file=$1
    local pyproject_version=$2
    echo "- simpleaible/pyproject.toml consistent with VERSION file?"
    if [[ "$pyproject_version" == "$version_file" ]]; then
        echo "  OK"
        return 0
    else
        echo "  FAIL simpleaible/pyproject.toml version ($pyproject_version) does not match VERSION file content ($version_file)."
        return 1
    fi
}


check_changelog() {
    local current_commit_tag=$1
    if [[ -n "$current_commit_tag" ]]; then
        echo "- Release tag found in changelog?"
        if grep -q "$current_commit_tag" docs/content/docs/changelog.mdx; then
            echo "  OK"
            return 0
        else
            echo "  FAIL, did you forgot to update changelog?"
            return 1
        fi
    fi
}
###############################################################################
# Test logic

assert_return_value() {
    local command="$1"
    local expected=$2
    local description="$3"

    # Run command and redirect output to /dev/null
    eval "$command" > /dev/null 2>&1
    local actual=$?

    if [ $actual -ne $expected ]; then
        echo "UNIT TEST FAILED: $command (expected: $expected, got: $actual) Check code logic"
        return 1
    fi
}

assert_return_value 'validate_version_format "1.1"' 1  || exit 1
assert_return_value 'validate_version_format "1.1.1"' 0 || exit 1
assert_return_value 'validate_version_format "1.1.1-dev"' 0 || exit 1
assert_return_value 'validate_version_format "1.1.1-dev1"' 1 || exit 1
assert_return_value 'check_version_after_last_tag "1.1.2-dev" "1.1.1"' 0 || exit 1
assert_return_value 'check_version_after_last_tag "1.1.2" "1.1.1"' 0 || exit 1
assert_return_value 'check_version_after_last_tag "1.1.1-dev" "1.1.1"' 1 || exit 1
assert_return_value 'check_version_after_last_tag "1.1.0" "1.1.1"' 1 || exit 1
assert_return_value 'check_cargo_version_match "1.1.1" "1.1.2"' 1 || exit 1
assert_return_value 'check_cargo_version_match "1.1.1" "1.1.1"' 0 || exit 1
assert_return_value 'check_cargo_version_match "1.1.1-dev" "1.1.1-dev"' 0 || exit 1
assert_return_value 'check_cargo_version_match "1.1.1" "1.1.1-dev"' 1 || exit 1
assert_return_value 'check_pyproject_version_match "1.1.1" "1.1.1"' 0 || exit 1
assert_return_value 'check_pyproject_version_match "1.1.1-dev" "1.1.1-dev"' 0 || exit 1
assert_return_value 'check_pyproject_version_match "1.1.1" "1.1.2"' 1 || exit 1
assert_return_value 'check_changelog "1.1.9" ' 1 || exit 1
assert_return_value 'check_changelog "0.7.0" ' 0 || exit 1
assert_return_value 'check_changelog "" ' 0 || exit 1
###############################################################################

# Perform checks with real repository versions

current_tag=$(git describe --exact-match --tags HEAD 2>/dev/null | sed 's/^v//')
latest_tag=$(git describe --tags --abbrev=0 | sed 's/^v//')
version_file=$(cat VERSION)
cargo_version=$(grep '^version = ' Cargo.toml | sed 's/version = "\(.*\)"/\1/')
pyproject_version=$(grep '^version = ' simpleaible/pyproject.toml | sed 's/version = "\(.*\)"/\1/')

validate_version_format "$version_file" || exit 1
check_cargo_version_match "$version_file" "$cargo_version" || exit 1
check_pyproject_version_match "$version_file" "$pyproject_version" || exit 1
if [[ -z "$current_tag" ]]; then
    check_version_after_last_tag "$version_file" "$latest_tag" || exit 1
fi
check_changelog "$current_tag"
