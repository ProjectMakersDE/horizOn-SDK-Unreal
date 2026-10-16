#!/usr/bin/env python3
"""Run native Unreal Chat automation and its isolated strict HTTP fixture.

Build the SDK in a dedicated HostProject first. No engine installation or downloads.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--host-project", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args()
    report = args.report.resolve()
    report.mkdir(parents=True, exist_ok=True)
    editor = args.engine / "Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
    if not editor.is_file() or not args.host_project.is_file():
        raise RuntimeError("Existing engine and independently built HostProject are required")
    marker = report / "fixture-ready"
    marker.unlink(missing_ok=True)
    environment = os.environ.copy()
    environment["MOCK_SERVER_READY_FILE"] = str(marker)
    race_marker = report / "delayed-sync-received"
    race_marker.unlink(missing_ok=True)
    environment["MOCK_CHAT_DELAY_MARKER"] = str(race_marker)
    environment["HORIZON_CHAT_FIXTURE_URL"] = "http://127.0.0.1:18822"
    environment["HORIZON_CHAT_REGION_B_URL"] = "http://127.0.0.1:18823"
    fixture = None
    engine = None
    with (report / "fixture.log").open("w") as fixture_log, (report / "editor-console.log").open("w") as editor_log:
        try:
            fixture = subprocess.Popen(["python3", str(Path(__file__).with_name("mock_chat_server.py"))], env=environment, stdout=fixture_log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 10
            while not marker.exists():
                if fixture.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError("Chat fixture failed before readiness")
                time.sleep(.05)
            # Never pre-probe fixture HTTP: its exact sequence is part of the gate.
            userdir = report / "isolated-user"
            engine = subprocess.Popen([str(editor), str(args.host_project.resolve()), "-unattended", "-nosplash", "-nullrhi", "-nosound", "-NoLiveCoding", "-NoTrace", "-UserDir=" + str(userdir), "-ExecCmds=Automation RunTests Horizon.Chat;Quit", "-TestExit=Automation Test Queue Empty", "-ReportExportPath=" + str(report), "-abslog=" + str(report / "editor.log")], env=environment, stdout=editor_log, stderr=subprocess.STDOUT)
            result = engine.wait(timeout=240)
            if result:
                raise RuntimeError(f"Native editor failed before fixture completion: {result}")
            fixture_result = fixture.wait(timeout=10)
            if result or fixture_result:
                raise RuntimeError(f"Native/fixture process failed: {result}/{fixture_result}")
            index = json.loads((report / "index.json").read_text(encoding="utf-8-sig"))
            tests = index.get("tests", [])
            expected = {"Horizon.Chat.NativeTransport", "Horizon.Chat.StrictModels"}
            actual = {item.get("fullTestPath") for item in tests}
            if actual != expected or any(item.get("state") != "Success" or item.get("errors", 0) for item in tests):
                raise RuntimeError("Native report did not contain both successful Chat tests with zero errors")
            count = sum(line.startswith("feature ") for line in (report / "fixture.log").read_text().splitlines())
            print(f"Native Chat: 2 tests passed; strict fixture passed, {count} validated HTTP requests")
        finally:
            for process in (engine, fixture):
                if process is not None and process.poll() is None:
                    process.terminate()
                    try: process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        process.kill(); process.wait(timeout=10)


if __name__ == "__main__": main()
