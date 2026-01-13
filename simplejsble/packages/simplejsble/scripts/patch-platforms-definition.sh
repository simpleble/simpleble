#!/bin/bash
#
# Patches NitroModules.podspec to use standard CocoaPods deployment_target syntax.
#
# Problem: react-native-nitro-modules uses non-standard `s.platforms = {...}` syntax
# which doesn't properly propagate deployment targets, causing build errors like:
# "Property with 'retain (or strong)' attribute must be of object type"
#
# Solution: Replace with standard `s.ios.deployment_target = ...` syntax.
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT_NAME="[simplejsble]"

PATHS=(
  "$SCRIPT_DIR/../../react-native-nitro-modules/NitroModules.podspec"
)

PODSPEC_PATH=""
for path in "${PATHS[@]}"; do
  if [ -f "$path" ]; then
    PODSPEC_PATH="$path"
    break
  fi
done

if [ -z "$PODSPEC_PATH" ]; then
  echo "$SCRIPT_NAME NitroModules.podspec not found, skipping patch"
  exit 0
fi

if grep -q "s\.ios\.deployment_target" "$PODSPEC_PATH" || grep -q "s\.macos\.deployment_target" "$PODSPEC_PATH"; then
  echo "$SCRIPT_NAME NitroModules.podspec already using standard syntax"
  exit 0
fi

if ! grep -q "s\.platforms" "$PODSPEC_PATH"; then
  echo "$SCRIPT_NAME NitroModules.podspec format not recognized, skipping patch"
  exit 0
fi

NEW_SYNTAX="s.ios.deployment_target = '15.8'
  s.macos.deployment_target = '13.0'
  s.tvos.deployment_target = '13.4'
  s.visionos.deployment_target = '1.0'"

sed -i.bak '/s\.platforms.*=.*{/,/}/c\
  s.ios.deployment_target = '"'"'15.8'"'"'\
  s.macos.deployment_target = '"'"'13.0'"'"'\
  s.tvos.deployment_target = '"'"'13.4'"'"'\
  s.visionos.deployment_target = '"'"'1.0'"'"'' "$PODSPEC_PATH"

rm -f "${PODSPEC_PATH}.bak"

echo "$SCRIPT_NAME ✓ Patched NitroModules.podspec platforms definition"
