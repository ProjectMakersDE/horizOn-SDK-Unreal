#!/usr/bin/env python3
"""Strict loopback-only Inbox/native-signup contract; no live accounts or secrets."""
import copy
import json
import os
import socket
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

USER = "00000000-0000-4000-8000-000000001006"
ENTRY = "broadcast:00000000-0000-4000-8000-000000001006"
ROOT = "/api/v1/app/inbox"


def snapshot(revision, broadcast=20, second=None, unchanged=False, claimed=False, read=False):
    item = {"id": ENTRY, "kind": "broadcast", "type": "reward", "reference": "fixture",
            "textKey": "fixture.reward", "parameters": {"name": "Fixture"},
            "createdAt": "2026-10-16T00:00:00Z", "expiresAt": "2026-10-17T00:00:00Z",
            "overdue": False, "read": read, "claimed": claimed,
            "attachment": {"values": [{"key": "gold", "amount": 7}]}}
    return {"userId": USER, "cursor": f"cursor_{revision}_{broadcast}", "revision": revision,
            "broadcastRevision": broadcast, "serverTime": f"2026-10-16T00:00:{second if second is not None else revision:02d}Z",
            "pollAfterSeconds": 30, "unchanged": unchanged, "entries": [] if unchanged else [item],
            "counts": {"reward": {"total": 1, "unread": int(not read), "claimable": int(not claimed)}},
            "limits": {"targeted": 50, "broadcasts": 500}}


def claim(revision, replayed=False):
    return {"entryId": ENTRY, "replayed": replayed, "inbox": snapshot(revision, claimed=True),
            "playerValues": {"userId": USER, "definitionVersion": 1, "revision": 30,
                "day": "2026-10-16", "values": [{"key": "gold", "balance": 100, "earnedToday": 0, "dailyCap": None}],
                "operationWindow": {"serverTime": "2026-10-16T00:00:30Z", "oldestAcceptedAt": "2026-09-16T00:00:30Z"}}}


def get(value, since=None, delay=0):
    return ("GET", ROOT, {"userId": [USER], **({"since": [since]} if since else {})}, None, 200, value, delay)


def post(action, value, status=200, delay=0):
    return ("POST", ROOT + "/" + action, {}, {"userId": USER, "entryId": ENTRY}, status, value, delay)


EXPECTED = [
    ("POST", "/api/v1/app/user-management/signup", {}, {"type": "ANONYMOUS", "username": "signup-503"}, 503, {"message": "uncertain signup"}, 0),
    ("POST", "/api/v1/app/user-management/signup", {}, {"type": "ANONYMOUS", "username": "signup-drop"}, 0, None, 0),
    get(snapshot(10)), get(snapshot(10, unchanged=True), "cursor_10_20"),
    get(snapshot(10, unchanged=True), "cursor_10_20"), get(snapshot(11)),
    get(snapshot(10)), get(snapshot(12)),
    get(snapshot(13), delay=.15), get(snapshot(14)),
    post("read", snapshot(15, read=True)),
    post("claim", claim(16)), post("claim", claim(16, True)),
    post("claim", claim(17), delay=.15),
    get(snapshot(15, 21)), get(snapshot(17, 19)), get(snapshot(17, 21, second=15)),
    get(snapshot(17, 21)),
    post("remove", {"code": "UNCLAIMED_REWARD", "message": "Claim valuable attachment first"}, 409),
    post("read", snapshot(18, 21, read=True), delay=.15),
    get(snapshot(19, 21), delay=.15),
]


class Handler(BaseHTTPRequestHandler):
    count = 0
    failed = None

    def do_GET(self): self.handle_contract("GET")
    def do_POST(self): self.handle_contract("POST")

    def send_json(self, status, value):
        payload = json.dumps(value).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def reject(self, message):
        type(self).failed = message
        print(message, flush=True)
        self.send_json(422, {"code": "FIXTURE_CONTRACT", "message": message})

    def handle_contract(self, method):
        if self.path == "/actuator/health":
            self.send_json(200, {"status": "UP"}); return
        body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", "0"))) or "null")
        if self.path == "/api/v1/app/user-management/signin":
            if body != {"type": "ANONYMOUS", "anonymousToken": "fixture-anonymous"}:
                self.reject("wrong signin payload"); return
            self.send_json(200, {"userId": USER, "username": "Fixture", "accessToken": "fixture-session-1006", "authStatus": "AUTHENTICATED", "isAnonymous": True, "isVerified": False}); return
        index = type(self).count
        type(self).count += 1
        if index >= len(EXPECTED): self.reject("unexpected extra request"); return
        verb, path, query, expected_body, status, response, delay = EXPECTED[index]
        url = urlparse(self.path)
        if (method, url.path, parse_qs(url.query), body) != (verb, path, query, expected_body):
            self.reject(f"request {index + 1} mismatched route, query or original payload"); return
        if self.headers.get("X-API-Key") != "fixture-key-1006" or (index > 1 and self.headers.get("Authorization") != "Bearer fixture-session-1006"):
            self.reject("wrong project/session context"); return
        time.sleep(delay)
        if not status:
            self.connection.shutdown(socket.SHUT_RDWR); self.connection.close(); return
        self.send_json(status, copy.deepcopy(response))

    def log_message(self, *_args): pass


def main():
    server = HTTPServer(("127.0.0.1", 18816), Handler)
    marker = os.environ.get("MOCK_SERVER_READY_FILE")
    if marker:
        with open(marker, "w") as handle: handle.write("ready\n")
    server.timeout = .2
    deadline = time.monotonic() + 420
    while Handler.count < len(EXPECTED) and not Handler.failed and time.monotonic() < deadline:
        server.handle_request()
    server.timeout = 1.5
    server.handle_request()  # A hidden retry or polling request must fail the gate.
    server.server_close()
    if Handler.failed or Handler.count != len(EXPECTED):
        print(f"Inbox contract failed: {Handler.failed}; requests={Handler.count}/{len(EXPECTED)}", flush=True)
        return 1
    print(f"Inbox native HTTP contract: {Handler.count} exact requests, signup attempts exactly one each", flush=True)
    return 0


if __name__ == "__main__": raise SystemExit(main())
