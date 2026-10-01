#!/usr/bin/env python3
"""Compile selected production manager methods against an in-memory UE boundary.

This exercises the actual anonymous signup/signin/restore and binary load code
without an installed UE project. It does not replace UnrealBuildTool validation.
Server contract: UserService.createAnonymousUser, SignUpResponse, SignInResponse,
AppCloudSaveController.loadCloudSave, inspected 2026-10-01.
"""
from pathlib import Path
import os
import subprocess
import tempfile


def extract(path, name):
    source = path.read_text()
    start = source.index("void " + name + "(")
    opening = source.index("{", start)
    depth = 1
    position = opening + 1
    while depth:
        depth += (source[position] == "{") - (source[position] == "}")
        position += 1
    return source[start:position]


root = Path(__file__).resolve().parents[1]
private = root / "Plugins/HorizonSDK/Source/HorizonSDK/Private"
auth = private / "Managers/HorizonAuthManager.cpp"
cloud = private / "Managers/HorizonCloudSaveManager.cpp"
methods = [extract(auth, "UHorizonAuthManager::" + name) for name in
           ["SignUpAnonymous", "SignInAnonymous", "RestoreAnonymousSession", "HandleAuthResponse", "CacheSession", "CheckAuth"]]
methods.append(extract(private / "Models/HorizonUserData.cpp", "FHorizonUserData::UpdateFromAuthResponse"))
methods.append(extract(cloud, "UHorizonCloudSaveManager::LoadBytes"))
http = private / "Http/HorizonHttpClient.cpp"
methods.extend(extract(http, "UHorizonHttpClient::" + name) for name in
               ["PostJsonForBinary", "ApplyHeaders"])
with tempfile.TemporaryDirectory(prefix="horizon-unreal-manager-test-") as temporary:
    folder = Path(temporary)
    (folder / "ManagerMethods.inc").write_text("\n\n".join(methods))
    binary = folder / "manager-test"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-I", str(folder),
                    "-I", str(root / "Plugins/HorizonSDK/Source/HorizonSDK/Public"),
                    str(root / "Tests/AuthCloudSaveRuntimeTest.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
