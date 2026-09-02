#!/usr/bin/env bash
set -euo pipefail

build_dir="${TMPDIR:-/tmp}/horizon-unreal-transport-test"
mkdir -p "$build_dir"
c++ -std=c++17 -pthread \
  -I Plugins/HorizonSDK/Source/HorizonSDK/Public \
  Tests/LeaderboardTransportContractTest.cpp \
  -o "$build_dir/leaderboard-transport-test"
"$build_dir/leaderboard-transport-test"
