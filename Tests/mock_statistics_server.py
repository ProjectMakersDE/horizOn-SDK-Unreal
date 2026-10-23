"""Exact JSON-number, bearer, replay and delayed-snapshot loopback contract for TASK-1005."""
import json
import os
import re
import sys
import threading
import time
from datetime import datetime, timedelta, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse
def send_json(handler,status,value):
    body=json.dumps(value,separators=(",",":")).encode()
    handler.send_response(status); handler.send_header("Content-Type","application/json");handler.send_header("Content-Length",str(len(body)));handler.end_headers();handler.wfile.write(body)
def report_rejection(message): print(message,file=sys.stderr)

PORT = 18806
MAX = 9007199254740991
utc = datetime.now(timezone.utc)
MONDAY = int((utc.replace(hour=0, minute=0, second=0, microsecond=0) + timedelta(days=7-utc.weekday(), seconds=1)).timestamp()*1000)
DEFINITIONS = [{"key": "kills", "name": "Kills", "mode": "sum", "reset": "none"}]

def snapshot(revision, value, version=1, definitions=True, at=None, operation=None, replay=False):
    result = {"userId": "00000000-0000-4000-8000-000000000005", "definitionVersion": version, "definitions": DEFINITIONS if definitions else None,
              "revision": revision, "values": [{"key": "kills", "value": value, "hasValue": True, "period": None}],
              "operationWindow": {"serverTimeUnixMillis": at or int(time.time()*1000), "maxAgeSeconds":86400, "maxFutureSkewSeconds":30}, "newlyUnlocked": []}
    if operation: result.update(operationId=operation, replayed=replay)
    return result

class Contract(BaseHTTPRequestHandler):
    count = 0
    failure = None
    operation = None
    lock = threading.Lock()
    def log_message(self, *_args): pass
    def do_GET(self):
        if self.path == "/actuator/health":
            send_json(self,200,{"status":"UP"});return
        self.handle_step("GET")
    def do_POST(self):
        if self.path == "/api/v1/app/user-management/signin":
            body=json.loads(self.rfile.read(int(self.headers.get("Content-Length","0"))))
            assert body.get("anonymousToken") == "fixture-anonymous"
            send_json(self,200,{"userId":"00000000-0000-4000-8000-000000000005","username":"StatisticsFixture","accessToken":"statistics-session","authStatus":"AUTHENTICATED"});return
        self.handle_step("POST")
    def handle_step(self, method):
        with self.lock:
            step = type(self).count + 1
            type(self).count = step
        try:
            assert step <= 15, "unexpected extra request"
            assert self.headers.get("X-API-Key") == "project-statistics", "project key"
            token = "statistics-session"
            assert self.headers.get("Authorization") == "Bearer " + token, "bearer session"
            url = urlparse(self.path)
            query = parse_qs(url.query)
            if step in (14, 15):
                assert method == "POST" and url.path == "/api/v1/app/statistics"
                body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", "0"))))
                assert body["userId"] == "00000000-0000-4000-8000-000000000005"
                assert body["updates"] == [{"key": f"stat{i}", "value": i} for i in range(201)]
                if step == 14: type(self).operation = body["operationId"]
                assert body["operationId"] == type(self).operation, "large replay keeps original ID"
                result = snapshot(4, 3, version=3 if step == 14 else 4, at=MONDAY+3,
                                  operation=body["operationId"], replay=step == 15)
                if step == 14:
                    result["definitions"] = [{"key": f"stat{i}", "name": "Stat", "mode": "sum", "reset": "none"} for i in range(201)]
                    result["values"] = [{"key": f"stat{i}", "value": i, "hasValue": True, "period": None} for i in range(201)]
                send_json(self, 200, result)
                return
            if step in (3,4,5):
                assert method == "POST" and url.path == "/api/v1/app/statistics", "POST path"
                body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", "0"))))
                assert body.get("userId") == "00000000-0000-4000-8000-000000000005", "player ID"
                operation = body.get("operationId")
                assert isinstance(operation, str) and re.fullmatch(r"[0-9]{13}:[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}", operation), "timestamp UUIDv4"
                if step == 3: type(self).operation = operation
                assert operation == type(self).operation, "original operation ID preserved"
                value = MAX if step in (3,4) else 1
                assert body.get("updates") == [{"key":"kills", "value":value}], "exact update payload"
                assert type(body["updates"][0]["value"]) is int, "JSON value must be integer, not float/string"
                if step == 5: send_json(self,409,{"code":"OPERATION_CONFLICT","message":"conflict"})
                else: send_json(self,200,snapshot(3,MAX,operation=operation,replay=step==4))
                return
            assert method == "GET", "GET method"
            if step == 6:
                assert url.path == "/api/v1/app/statistics/definitions" and query == {"definitionVersion":["1"]}, "conditional definitions"
                send_json(self,200,{"definitionVersion":2,"definitions":DEFINITIONS})
                return
            expected_query = {"userId":["00000000-0000-4000-8000-000000000005"]}
            if step in (2,7,8,9,10,12,13): expected_query["definitionVersion"]=["1" if step==2 else "2"]
            assert url.path == "/api/v1/app/statistics" and query == expected_query, "snapshot query " + str(query)
            if step == 1: result=snapshot(1,-MAX)
            elif step == 2: result=snapshot(2,2,definitions=False)
            elif step == 7: result=snapshot(2,99,2)
            elif step == 8: result=snapshot(3,3,2,at=MONDAY)
            elif step == 9: result=snapshot(3,99,2,at=MONDAY-2000)
            elif step == 10:
                time.sleep(.25)
                result=snapshot(3,3,2,definitions=False,at=MONDAY)
            elif step == 11: result=snapshot(3,3,2,at=MONDAY)
            elif step == 12:
                result=snapshot(3,3,2,at=MONDAY)
                result["values"][0]["value"]="3"
            elif step == 13:
                result=snapshot(3,3,2,at=MONDAY)
                # A double would round this fraction to the accepted MAX integer.
                raw=json.dumps(result,separators=(",",":")).replace('"value":3,','"value":9007199254740991.1,').encode()
                self.send_response(200);self.send_header("Content-Type","application/json");self.send_header("Content-Length",str(len(raw)));self.end_headers();self.wfile.write(raw);return
            else: raise AssertionError("unexpected step")
            send_json(self,200,result)
        except Exception as error:
            type(self).failure = f"request {step}: {error}"
            report_rejection(type(self).failure)
            send_json(self,422,{"code":"CONTRACT_MISMATCH","message":str(error)})

def main():
    server=ThreadingHTTPServer(("127.0.0.1",PORT),Contract)
    ready=os.environ.get("MOCK_SERVER_READY_FILE")
    if ready:
        with open(ready,"w") as handle: handle.write("ready\n")
    server.timeout=.2
    deadline=time.monotonic()+420
    while Contract.count < 15 and not Contract.failure and time.monotonic()<deadline: server.handle_request()
    server.timeout=1
    server.handle_request()
    server.server_close()
    if Contract.count != 15 or Contract.failure:
        print(Contract.failure or f"expected 15 requests, saw {Contract.count}",file=sys.stderr)
        return 1
    print("Unreal statistics server observed exact integer, scope and stable retry contract")
    return 0

if __name__ == "__main__": raise SystemExit(main())
