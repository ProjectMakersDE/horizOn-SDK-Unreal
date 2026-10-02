#!/usr/bin/env bash
set -euo pipefail

build_dir="$(mktemp -d "${TMPDIR:-/tmp}/horizon-unreal-transport-test.XXXXXX")"
trap 'rm -rf "$build_dir"' EXIT
c++ -std=c++17 -pthread \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/LeaderboardTransportContractTest.cpp \
  -o "$build_dir/leaderboard-transport-test"
"$build_dir/leaderboard-transport-test"
c++ -std=c++17 \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/GiftCodeTransportContractTest.cpp \
  -o "$build_dir/gift-code-transport-test"
"$build_dir/gift-code-transport-test"
c++ -std=c++17 \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/PlayerProfileTransportContractTest.cpp \
  -o "$build_dir/player-profile-transport-test"
"$build_dir/player-profile-transport-test"
c++ -std=c++17 \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/ValidatedActionsTransportContractTest.cpp \
  -o "$build_dir/validated-actions-transport-test"
"$build_dir/validated-actions-transport-test"

# The production Validated Actions models against an in-memory UE boundary: the engine headers
# they include are empty stand-ins, the test itself defines the few UE types the models use.
model_boundary="$build_dir/ue-boundary"
mkdir -p "$model_boundary/Dom"
: > "$model_boundary/CoreMinimal.h"
: > "$model_boundary/HorizonValidatedActions.generated.h"
: > "$model_boundary/Dom/JsonObject.h"
: > "$model_boundary/Dom/JsonValue.h"
c++ -std=c++17 \
  -I "$model_boundary" \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/ValidatedActionsModelParseTest.cpp \
  -o "$build_dir/validated-actions-model-test"
"$build_dir/validated-actions-model-test"

python3 Tests/run_auth_cloud_save_runtime_test.py
