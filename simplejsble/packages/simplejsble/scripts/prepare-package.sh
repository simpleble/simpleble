#!/bin/bash
set -e

# Get script directory and calculate paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${PACKAGE_DIR}/../../.." && pwd)"

# Directories to clean before packing (native build artifacts only, not lib/)
DIRS_TO_CLEAN=(
  "android/build"
  "android/.cxx"
  "android/.gradle"
  "ios/build"
)

# Directories/files to copy from repo root
ITEMS_TO_COPY=(
  "simpleble"
  "simpledroidbridge"
  "cmake"
  "dependencies"
  "VERSION"
)

echo "Preparing package for npm publish..."
echo "Package dir: ${PACKAGE_DIR}"
echo "Repo root: ${REPO_ROOT}"

# Clean build artifacts first
echo ""
echo "Cleaning build artifacts..."
for dir in "${DIRS_TO_CLEAN[@]}"; do
  dir_path="${PACKAGE_DIR}/${dir}"
  if [ -d "${dir_path}" ]; then
    rm -rf "${dir_path}"
    echo "✓ Cleaned: ${dir}"
  fi
done
echo ""

# Copy items from repo root
for item in "${ITEMS_TO_COPY[@]}"; do
  src="${REPO_ROOT}/${item}"
  dest="${PACKAGE_DIR}/${item}"

  if [ ! -e "${src}" ]; then
    echo "Warning: ${src} does not exist, skipping..."
    continue
  fi

  # Remove existing copy if it exists
  if [ -e "${dest}" ]; then
    rm -rf "${dest}"
  fi

  # Copy directory or file
  if [ -d "${src}" ]; then
    cp -R "${src}" "${dest}"
    echo "✓ Copied directory: ${item}"
  else
    cp "${src}" "${dest}"
    echo "✓ Copied file: ${item}"
  fi
done

echo "Package preparation complete!"
