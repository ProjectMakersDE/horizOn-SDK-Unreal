#!/usr/bin/env python3
"""Strict artificial item gifts consumer contract; no real accounts or persistence."""
import copy
import datetime as dt
import json
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse
from uuid import UUID

SENDER = "20000000-0000-4000-8000-000000001140"
RECIPIENT = "30000000-0000-4000-8000-000000001140"
INSTANCE = "40000000-0000-4000-8000-000000001140"
GIFT = "50000000-0000-4000-8000-000000001140"
UNKNOWN = "60000000-0000-4000-8000-000000009999"
OFFER_ENTRY = "message:70000000-0000-4000-8000-000000001140"
RETURN_ENTRY = "message:70000000-0000-4000-8000-000000001141"
ROOT = "/api/v1/app/item-gifts"
BASE = dt.datetime(2026, 11, 16, 10, tzinfo=dt.timezone.utc)
START = time.monotonic()
SAVED = {}
SAVED_RAW = {}
def iso(value): return value.isoformat(timespec="milliseconds").replace("+00:00", "Z")
def now(): return iso(BASE + dt.timedelta(seconds=time.monotonic() - START))
def stamp(): return iso(BASE)
def limits(): return {"openGiftsPerPlayer": 50, "retainedGiftsPerProject": 10000, "projectWritesPerMinute": 120, "historyPerPlayer": 20, "receivedPageSize": 50, "playerWriteIntervalMillis": 1000, "expirySeconds": 604800, "terminalRetentionSeconds": 604800, "operationRetentionSeconds": 2592000}
def config_view(currency="1coin.gold"): return {"config": {"revision": 3, "enabled": True, "currencyKey": currency, "fee": 5}, "limits": limits(), "serverTime": now()}
def gift(status="offered", revision=2):
    terminal = status in ("claimed", "returned")
    declined = status in ("return_pending", "return_delivered", "returned")
    return {"id": GIFT, "revision": revision, "senderId": SENDER, "recipientId": RECIPIENT, "returnOwnerId": SENDER, "itemReference": {"instanceId": INSTANCE, "itemKey": "hat"}, "currencyKey": "1coin.gold", "fee": 5, "configRevision": 3, "status": status, "createdAt": stamp(), "expiresAt": iso(BASE + dt.timedelta(days=7)), "terminalAt": stamp() if terminal else None, "declinedAt": stamp() if declined else None, "offerEntryId": OFFER_ENTRY, "returnEntryId": RETURN_ENTRY if status in ("return_delivered", "returned") else None, "ownerUnavailable": False}
def state(user=SENDER, revision=1, sent=None, received=None):
    return {"state": {"userId": user, "revision": revision, "sent": sent or [], "history": [], "received": received or [], "operationWindow": {"serverTime": now(), "acceptAfter": iso(BASE - dt.timedelta(days=30))}, "limits": limits()}, "serverTime": now()}
def original(): return {"instanceId": INSTANCE, "itemKey": "hat", "properties": {"power": 9007199254740991}, "equipped": False, "createdAt": stamp()}
def inventory(revision=9, instances=None): return {"userId": SENDER, "revision": revision, "instances": instances or [], "limits": {"catalog": 6000, "inventory": 250, "receipts": 80}, "serverTime": now()}
def values(revision=5): return {"userId": SENDER, "day": "2026-11-16", "definitionVersion": 1, "revision": revision, "values": [{"key": "1coin.gold", "balance": 95, "earnedToday": 0, "dailyCap": None}], "operationWindow": {"serverTime": now(), "oldestAcceptedAt": iso(BASE - dt.timedelta(days=30))}}
def receipt():
    body = SAVED["send"]
    return {"operationId": body["operationId"], "issuedAt": body["issuedAt"], "giftId": GIFT, "giftRevision": 2, "recordedAt": stamp(), "recipientId": RECIPIENT, "itemReference": {"instanceId": INSTANCE, "itemKey": "hat"}, "currencyKey": "1coin.gold", "fee": 5, "configRevision": 3}
def entry(entry_id, kind, key, parameters, direction, claimed=False):
    return {"id": entry_id, "kind": "message", "type": kind, "reference": GIFT, "textKey": key, "parameters": parameters, "createdAt": stamp(), "expiresAt": iso(BASE + dt.timedelta(days=7)), "read": claimed, "claimed": claimed, "overdue": False, "attachment": {"values": [], "itemGift": {"giftId": GIFT, "instanceId": INSTANCE, "itemKey": "hat", "direction": direction}}}
def inbox(user, row, kind, claimed=False):
    return {"userId": user, "cursor": "gift-claimed" if claimed else "gift-open", "revision": 2 if claimed else 1, "broadcastRevision": 1, "serverTime": now(), "pollAfterSeconds": 120, "unchanged": False, "entries": [row], "counts": {kind: {"total": 1, "unread": 0 if claimed else 1, "claimable": 0 if claimed else 1}}, "limits": {"targeted": 50, "broadcasts": 500}}
def offer_inbox(): return inbox(RECIPIENT, entry(OFFER_ENTRY, "item_gift", "horizon.item_gifts.offer", {"senderId": SENDER}, "offer"), "item_gift")
def claim():
    row = entry(RETURN_ENTRY, "item_gift_return", "horizon.item_gifts.return", {"recipientId": RECIPIENT, "reason": "declined"}, "return", True)
    return {"entryId": RETURN_ENTRY, "replayed": False, "inbox": inbox(SENDER, row, "item_gift_return", True), "playerValues": values(6), "items": [{"instanceId": INSTANCE, "itemKey": "hat"}]}
def q(user=SENDER): return {"userId": [user]}
EXPECTED = []
def expect(method, path, response, query=None, body=None, user=SENDER, status=200, save=None, repeat=None, delay=0):
    EXPECTED.append(dict(method=method, path=path, response=response, query=query or {}, body=body, user=user, status=status, save=save, repeat=repeat, delay=delay))
expect("GET", ROOT + "/config", config_view, q())
expect("GET", ROOT, state, q())
expect("GET", "/api/v1/app/items", lambda: inventory(instances=[original()]), q())
expect("POST", ROOT, {"code": "STATE_CONFLICT"}, body="send", status=503, save="send", delay=.2)
expect("GET", ROOT + "/operations/__SEND__", lambda: {"receipt": receipt(), "serverTime": now()}, q())
expect("GET", "/api/v1/app/items", {"code": "STATE_CONFLICT"}, q(), status=409)
expect("GET", "/api/v1/app/items", lambda: inventory(10), q())
expect("GET", "/api/v1/app/player-values", values, q())
expect("GET", ROOT, lambda: state(revision=2, sent=[gift()]), q())
expect("POST", ROOT, lambda: {"receipt": receipt(), "gift": gift("claimed", 3), "replayed": True, "serverTime": now()}, body="send", repeat="send")
expect("POST", ROOT, {"code": "OPERATION_CONFLICT"}, body="changed", status=409)
expect("GET", ROOT + "/operations/" + UNKNOWN, {"code": "OPERATION_NOT_FOUND"}, q(), status=404)
expect("GET", ROOT + "/" + GIFT, lambda: {"gift": gift(), "serverTime": now()}, q(RECIPIENT), user=RECIPIENT)
expect("GET", "/api/v1/app/inbox", offer_inbox, q(RECIPIENT), user=RECIPIENT)
expect("POST", ROOT + "/" + GIFT + "/decline", lambda: {"gift": gift("return_delivered", 4), "replayed": False, "returnDelivery": "delivered", "serverTime": now()}, body={"userId": RECIPIENT, "expectedGiftRevision": 2}, user=RECIPIENT)
expect("POST", ROOT + "/" + GIFT + "/decline", lambda: {"gift": gift("return_delivered", 4), "replayed": True, "returnDelivery": "delivered", "serverTime": now()}, body={"userId": RECIPIENT, "expectedGiftRevision": 2}, user=RECIPIENT)
expect("POST", "/api/v1/app/inbox/claim", claim, body={"userId": SENDER, "entryId": RETURN_ENTRY})
expect("GET", "/api/v1/app/items", lambda: inventory(11, [original()]), q())
expect("GET", ROOT + "/" + GIFT, lambda: {"gift": gift("returned", 5), "serverTime": now()}, q())
expect("GET", ROOT + "/config", lambda: config_view(None), q())
expect("GET", ROOT, lambda: state(revision=6), q())
expect("GET", ROOT, lambda: state(revision=5), q())
expect("GET", ROOT, lambda: state(revision=6), q(), delay=.2)

class Handler(BaseHTTPRequestHandler):
    index = 0
    failed = False
    lock = threading.Lock()
    bootstraps = 0
    health = 0
    def log_message(self, *_): pass
    def do_GET(self): self.handle_contract()
    def do_POST(self): self.handle_contract()
    def reply(self, status, value):
        encoded = json.dumps(value, ensure_ascii=True, separators=(",", ":")).encode()
        self.send_response(status); self.send_header("Content-Type", "application/json"); self.send_header("Content-Length", str(len(encoded))); self.end_headers(); self.wfile.write(encoded)
    def handle_contract(self):
        try:
            if self.path == "/actuator/health" or self.path == "/api/v1/app/user-management/signin":
                assert self.headers.get("X-API-Key") == "gift-fixture-key"
                if self.path == "/actuator/health":
                    assert self.command == "GET" and type(self).health == 0
                    type(self).health += 1
                    self.reply(200, {"status": "UP"}); return
                assert self.command == "POST"
                role = ["sender", "recipient", "sender"][type(self).bootstraps]
                body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", "0"))))
                assert body == {"type": "ANONYMOUS", "anonymousToken": "gift-" + role + "-anonymous"}
                assert type(self).index == [0, 12, 16][type(self).bootstraps], ("bootstrap order", type(self).index)
                type(self).bootstraps += 1
                self.reply(200, {"userId": SENDER if role == "sender" else RECIPIENT, "accessToken": "gift-" + role + "-session", "username": "Fixture", "authStatus": "AUTHENTICATED", "isAnonymous": True, "isVerified": False})
                return
            with self.lock:
                step = type(self).index
                assert step < len(EXPECTED), "Unexpected extra transport attempt"
                expected = EXPECTED[step]; type(self).index += 1
            path = expected["path"].replace("__SEND__", SAVED.get("send", {}).get("operationId", ""))
            actual = urlparse(self.path)
            assert self.command == expected["method"] and actual.path == path, (step + 1, self.command, actual.path, expected["method"], path)
            assert parse_qs(actual.query) == expected["query"], (step + 1, parse_qs(actual.query), expected["query"])
            assert self.headers.get("X-API-Key") == "gift-fixture-key"
            assert self.headers.get("Authorization") == "Bearer gift-" + ("sender" if expected["user"] == SENDER else "recipient") + "-session"
            raw = self.rfile.read(int(self.headers.get("Content-Length", "0")))
            body = json.loads(raw) if raw else None
            target = expected["body"]
            if isinstance(target, str):
                assert str(UUID(body["operationId"])) == body["operationId"]
                assert iso(dt.datetime.fromisoformat(body["issuedAt"].replace("Z", "+00:00"))) == body["issuedAt"]
                original_body = {"userId": SENDER, "operationId": body["operationId"], "issuedAt": body["issuedAt"], "recipientId": RECIPIENT, "instanceId": INSTANCE, "expectedInventoryRevision": 9, "expectedConfigRevision": 4 if target == "changed" else 3}
                assert body == original_body, (step + 1, body, original_body)
                if expected["repeat"]: assert raw == SAVED_RAW[expected["repeat"]], "Original bytes changed on retry"
                if target == "changed": assert body["operationId"] == SAVED["send"]["operationId"] and body["issuedAt"] == SAVED["send"]["issuedAt"]
                if expected["save"]: SAVED[expected["save"]] = copy.deepcopy(body); SAVED_RAW[expected["save"]] = raw
            else: assert body == target, (step + 1, body, target)
            if expected["delay"]: time.sleep(expected["delay"])
            self.reply(expected["status"], expected["response"]() if callable(expected["response"]) else expected["response"])
        except (BrokenPipeError, ConnectionResetError): pass
        except Exception as error:
            type(self).failed = True
            print("FAIL item gifts exact fixture:", repr(error), flush=True)
            self.send_error(500)

def main():
    server = ThreadingHTTPServer(("127.0.0.1", int(os.environ.get("HORIZON_ITEM_GIFTS_FIXTURE_PORT", "20140"))), Handler)
    server.daemon_threads = True
    threading.Thread(target=server.serve_forever, daemon=True).start()
    if os.environ.get("MOCK_SERVER_READY_FILE"):
        with open(os.environ["MOCK_SERVER_READY_FILE"], "w") as file: file.write("ready")
    deadline = time.monotonic() + 240
    while Handler.index < len(EXPECTED) and not Handler.failed and time.monotonic() < deadline: time.sleep(.05)
    time.sleep(.5); server.shutdown(); server.server_close()
    assert Handler.bootstraps == 3 and Handler.health == 1, ("Native explicit session bootstrap", Handler.bootstraps, Handler.health)
    assert not Handler.failed and Handler.index == len(EXPECTED), ("Missing or invalid requests", Handler.index, len(EXPECTED))
    print(f"PASS item gifts strict HTTP: {len(EXPECTED)} exact requests, all six gift routes, original send bytes and recovery, decline replay and Inbox return claim", flush=True)

if __name__ == "__main__": main()
