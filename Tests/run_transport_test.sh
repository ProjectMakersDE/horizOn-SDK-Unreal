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

python3 Tests/run_auth_cloud_save_runtime_test.py
