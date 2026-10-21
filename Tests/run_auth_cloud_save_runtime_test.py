#!/usr/bin/env python3
"""Compile selected production manager methods against an in-memory UE boundary.

This exercises the actual anonymous signup/signin/restore, binary load and News code
without an installed UE project. It does not replace UnrealBuildTool validation.
Server contract: UserService.createAnonymousUser, SignUpResponse, SignInResponse,
AppCloudSaveController.loadCloudSave, inspected 2026-10-01. Cloud save revisions
(X-Cloud-Save-Revision, If-Match, 409): AppCloudSaveController and
CloudSaveExceptionHandler, inspected 2026-10-11.
"""
from pathlib import Path
import os
import subprocess
import tempfile


def extract(path, name, signature=None):
    source = path.read_text()
    start = source.index(signature or "void " + name + "(")
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
news = private / "Managers/HorizonNewsManager.cpp"
methods = [extract(auth, "UHorizonAuthManager::" + name) for name in
           ["SignUpAnonymous", "SignInAnonymous", "RestoreAnonymousSession", "HandleAuthResponse", "CacheSession", "CheckAuth"]]
methods.append(extract(private / "Models/HorizonUserData.cpp", "FHorizonUserData::UpdateFromAuthResponse"))
methods.append(extract(private / "Models/HorizonNetworkResponse.cpp", None,
                       "FString FHorizonNetworkResponse::GetHeader("))
methods.extend(extract(cloud, None, signature) for signature in
               ["static bool TryGetCloudSaveBool(",
                "static bool TryGetCloudSaveString(",
                "static FHorizonCloudSaveSnapshot CloudSaveSnapshotFrom(",
                "static FHorizonCloudSaveWriteResult CloudSaveWriteResultFrom(",
                "static FString CheckConditionalSave(",
                "static TMap<FString, FString> CloudSaveIfMatch(",
                "static void LogCloudSaveWrite("])
methods.extend(extract(cloud, "UHorizonCloudSaveManager::" + name) for name in
               ["LoadBytes", "LoadSnapshot", "LoadBytesSnapshot"])
methods.append(extract(cloud, None, "void UHorizonCloudSaveManager::Save(const FString& Data, int64"))
methods.append(extract(cloud, None, "void UHorizonCloudSaveManager::SaveBytes(const TArray<uint8>& Data, int64"))
methods.append(extract(cloud, None, "void UHorizonCloudSaveManager::Save(const FString& Data, FOnRequestComplete"))
methods.append(extract(cloud, None, "void UHorizonCloudSaveManager::SaveBytes(const TArray<uint8>& Data, FOnRequestComplete"))
http = private / "Http/HorizonHttpClient.cpp"
methods.extend(extract(http, "UHorizonHttpClient::" + name) for name in
               ["PostJson", "PostJsonForBinary", "PostBinary", "ApplyHeaders"])
methods.append(extract(news, "UHorizonNewsManager::LoadNews"))
methods.append(extract(private / "Models/HorizonNewsEntry.cpp", None,
                       "FHorizonNewsEntry FHorizonNewsEntry::FromJson("))
with tempfile.TemporaryDirectory(prefix="horizon-unreal-manager-test-") as temporary:
    folder = Path(temporary)
    (folder / "ManagerMethods.inc").write_text("\n\n".join(methods))
    binary = folder / "manager-test"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-I", str(folder),
                    "-I", str(root / "Plugins/HorizonSDK/Source/HorizonSDK/Public"),
                    str(root / "Tests/AuthCloudSaveRuntimeTest.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
