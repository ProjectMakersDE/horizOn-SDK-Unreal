#!/usr/bin/env python3
"""Strict artificial native transport proof on reserved Unreal port 18817."""
import json
import os
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

JSON = {"intent": "original"}
GIFT = "/api/v1/app/gift-codes/redeem"
BASE = "/fixture/"
def item(route, body, accept="", status=429, response=None, after="1", wait=False):
    return (route, body, accept, status, response, after, wait)

EXPECTED = [
    item(BASE+"global/json", JSON), item(BASE+"global/json", JSON, status=200, response={"ok": True}, wait=True),
    item(BASE+"feature/json", JSON, response={"code": "FUTURE_FEATURE_POLICY", "message": "feature json"}, after="100000"),
    item(BASE+"feature/binary", bytes([1,2,3]), response={"code": "RUN_RATE_LIMITED", "message": "feature binary"}, after="100000"),
    item(BASE+"feature/negotiate", JSON, "application/octet-stream", response={"code": " ", "message": "feature negotiated"}, after="100000"),
    item(BASE+"global/binary", bytes([1,2,3]), response={"code": ""}), item(BASE+"global/binary", bytes([1,2,3]), status=200, response=bytes([9,8,7]), wait=True),
    item(BASE+"global/negotiate", JSON, "application/octet-stream", response={"code": None}), item(BASE+"global/negotiate", JSON, "application/octet-stream", 200, bytes([4,5]), wait=True),
    item(BASE+"exhaust", JSON), item(BASE+"exhaust", JSON, wait=True), item(BASE+"exhaust", JSON, wait=True),
    item(GIFT, {"code": "legacy"}, status=503, response={"message": "unknown legacy outcome"}),
    item(GIFT, {"code": "blank", "idempotencyId": "\u00a0"}, status=503, response={"message": "blank is legacy"}),
    item(GIFT, {"code": "explicit", "idempotencyId": "original-id"}, status=503, response={"message": "uncertain explicit response"}),
    item(GIFT, {"code": "explicit", "idempotencyId": "original-id"}, status=200, response={"ok": True}),
    item("/api/v1/app/user-management/signup", {"type": "ANONYMOUS"}, "application/octet-stream", 503, {"message": "uncertain signup"}),
    item(BASE+"session-switch", JSON),
]

class Handler(BaseHTTPRequestHandler):
    count = 0
    failed = None
    last_request = 0
    intervals = []
    def do_GET(self):
        if self.path != "/actuator/health": self.reject("unexpected GET"); return
        self.send_json(200, {"status": "UP"})
    def send_json(self, status, value, after=""):
        payload = b"" if value is None else value if isinstance(value, bytes) else json.dumps(value).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/octet-stream" if isinstance(value, bytes) else "application/json")
        self.send_header("Content-Length", str(len(payload)))
        if after: self.send_header("Retry-After", after)
        self.end_headers(); self.wfile.write(payload)
    def reject(self, message):
        type(self).failed = message
        print(message, flush=True)
        self.send_json(422, {"code": "FIXTURE_CONTRACT", "message": message})
    def do_POST(self):
        index = type(self).count; type(self).count += 1
        if index >= len(EXPECTED): self.reject("unexpected retry or extra request"); return
        path, body, accept, status, response, after, wait = EXPECTED[index]
        raw = self.rfile.read(int(self.headers.get("Content-Length", "0")))
        actual = raw if isinstance(body, bytes) else json.loads(raw)
        now = time.monotonic()
        if wait:
            interval = now-type(self).last_request; type(self).intervals.append(interval)
            if interval < .99 or interval > 1.5:
                self.reject(f"retry wait outside server minimum +20%/scheduler tolerance: {interval:.3f}s"); return
        type(self).last_request = now
        if self.path != path or actual != body or self.headers.get("X-API-Key") != "fixture-key-996" or self.headers.get("Authorization") != "Bearer fixture-session-996":
            self.reject(f"request {index+1} lost original route, bytes/JSON or identity"); return
        if accept and self.headers.get("Accept") != accept: self.reject("binary negotiation lost across retry"); return
        if path == BASE+"global/json" and self.headers.get("If-Match") != "17": self.reject("conditional header lost across retry"); return
        self.send_json(status, response, after)
    def log_message(self, *_args): pass

def main():
    server = HTTPServer(("127.0.0.1", 18817), Handler)
    marker = os.environ.get("MOCK_SERVER_READY_FILE")
    if marker:
        with open(marker, "w") as handle: handle.write("ready\n")
    server.timeout = .2; deadline = time.monotonic()+420
    while Handler.count < len(EXPECTED) and not Handler.failed and time.monotonic() < deadline: server.handle_request()
    server.timeout = 1.5; server.handle_request(); server.server_close()
    if Handler.failed or Handler.count != len(EXPECTED):
        print(f"Rate proof failed: {Handler.failed}; requests={Handler.count}/{len(EXPECTED)}", flush=True); return 1
    print(json.dumps({"requests": Handler.count, "globalRetryIntervalsSeconds": Handler.intervals, "singleAttemptLegacyAndSignup": True, "sessionSwitchRetrySuppressed": True}), flush=True)
    return 0
if __name__ == "__main__": raise SystemExit(main())
