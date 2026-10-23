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

c++ -std=c++17 \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/StatisticsTransportContractTest.cpp \
  -o "$build_dir/statistics-transport-test"
"$build_dir/statistics-transport-test"

c++ -std=c++17 \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/SignupRetryContractTest.cpp \
  -o "$build_dir/signup-retry-contract-test"
"$build_dir/signup-retry-contract-test"

c++ -std=c++17 \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/RetryContractTest.cpp \
  -o "$build_dir/retry-contract-test"
"$build_dir/retry-contract-test"

c++ -std=c++17 \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/GroupsTransportContractTest.cpp \
  -o "$build_dir/groups-transport-test"
"$build_dir/groups-transport-test"

c++ -std=c++17 \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/ItemsTransportContractTest.cpp \
  -o "$build_dir/items-transport-test"
"$build_dir/items-transport-test"

c++ -std=c++17 \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/SharedObjectsTransportContractTest.cpp \
  -o "$build_dir/shared-objects-transport-test"
"$build_dir/shared-objects-transport-test"

# Async Matches metadata/cursor contract, independent of an engine installation.
c++ -std=c++17 -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
    Tests/MatchesTransportContractTest.cpp -o "$build_dir/matches-transport-test"
"$build_dir/matches-transport-test"

# Strict scalar-counted chat text and immutable input bounds.
c++ -std=c++17 -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
    Tests/ChatTransportContractTest.cpp -o "$build_dir/chat-transport-test"
"$build_dir/chat-transport-test"

# Marketplace lists are bounded by the transport frame, independently of operator quotas.
c++ -std=c++17 -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
    Tests/MarketplaceTransportContractTest.cpp -o "$build_dir/marketplace-transport-test"
"$build_dir/marketplace-transport-test"
