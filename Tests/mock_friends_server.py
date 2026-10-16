#!/usr/bin/env python3
"""Strict artificial Friends contract, bound only to 127.0.0.1:18815."""
import base64,copy,json,os,socket,time,uuid
from http.server import BaseHTTPRequestHandler,HTTPServer
from urllib.parse import parse_qs,urlparse
U='00000000-0000-4000-8000-000000001009'
B='00000000-0000-4000-8000-000000002009';C='00000000-0000-4000-8000-000000003009'
D='00000000-0000-4000-8000-000000004009';E='00000000-0000-4000-8000-000000005009'
F='00000000-0000-4000-8000-000000006009';G='00000000-0000-4000-8000-000000007009'
R1='10000000-0000-4000-8000-000000001009';R2='20000000-0000-4000-8000-000000001009'
R3='30000000-0000-4000-8000-000000001009';R4='40000000-0000-4000-8000-000000001009'
ROOT='/api/v1/app/friends'
def code(user):return 'H-'+base64.urlsafe_b64encode(uuid.UUID(user).bytes).decode().rstrip('=')
def display(name=None):return {'name':name,'profile':{'avatarId':None,'frameId':None,'badges':[]}}
def friend(user,known=False):return {'userId':user,'since':'2026-10-16T00:00:01Z','display':display('Public friend'),'lastActiveAt':'2026-10-15T00:00:00Z' if known else None}
def request(user,rid):return {'userId':user,'requestId':rid,'createdAt':'2026-10-16T00:00:01Z','display':display()}
def snapshot(revision,second=None):
    incoming=[request(B,R1),request(E,R4)] if revision<=2 else [request(B,R2),request(E,R4)] if revision==5 else [request(E,R4)] if revision==6 else []
    outgoing=[request(C,R3)] if revision==1 else [request(C,R3),request(F,R2)] if revision<8 else [request(F,R2)] if revision<10 else []
    friends=[friend(D,True),friend(B)] if revision in (6,7,8) else [friend(D,True)]
    if revision>=13:
        friends=[friend(str(uuid.UUID(int=100000+index)),index==0) for index in range(20)]
        if revision==13:friends.append(friend(D))
    return {'userId':U,'revision':revision,'friendCode':code(U),'friends':friends,'incoming':incoming,'outgoing':outgoing,
            'blocked':[{'userId':user,'blockedAt':'2026-10-16T00:00:01Z'} for user in ([G,F] if revision==10 else [G])],
            'limits':{'maxFriends':20 if revision>=13 else 50,'maxIncomingRequests':20,'maxOutgoingRequests':20,'maxBlocked':100},
            'serverTime':f'2026-10-16T00:00:{second if second is not None else revision+1:02d}Z'}
def get(revision,second=None,delay=0):return ('GET',ROOT,{'userId':[U]},None,200,snapshot(revision,second),delay)
def lookup(query,user=B,delay=0,status=200):
    return ('GET',ROOT+'/find',{'userId':[U],'query':[query]},None,status,{'userId':user,'friendCode':code(user),'display':display()} if status==200 else {'code':'PLAYER_NOT_FOUND','message':'Player not found'},delay)
def post(action,target,expected,reply=None,status=200,rid=None,delay=0):
    body={'userId':U,'targetUserId':target,'expectedRevision':expected}
    if rid is not None:body['requestId']=rid
    return ('POST',ROOT+'/'+action,{},body,status,reply,delay)
def failure(code):return {'code':code,'message':'Explicit reconciliation required'}
bad=snapshot(17,40);bad['revision']='__ORIGINAL_FRACTION__'
EXPECTED=[
 get(1),lookup(B),lookup(code(B)),post('request',F,1,snapshot(2)),
 post('accept',B,2,None,0,R1),post('accept',B,2,failure('REVISION_CONFLICT'),409,R1),
 get(5),post('accept',B,5,failure('FRIEND_REQUEST_NOT_FOUND'),404,R1),post('accept',B,5,snapshot(6),200,R2),
 post('decline',E,6,snapshot(7),200,R4),post('cancel',C,7,snapshot(8),200,R3),post('remove',B,8,snapshot(9)),
 post('block',F,9,snapshot(10)),post('unblock',F,10,snapshot(11)),post('request',E,11,failure('INBOX_FULL'),409),
 lookup(E,E,status=404),post('block',E,11,failure('RELATIONSHIP_NOT_ALLOWED'),403),
 get(11,20),get(10,21),get(9,22),get(11,23),get(11,5),get(12,24,.15),get(12,25),get(13,26),
 post('remove',D,13,snapshot(14,27)),get(15,28,.15),get(15,29),get(16,30,.15),get(16,31),lookup(B,delay=.15),
 get(17,33),('GET',ROOT,{'userId':[U]},None,200,bad,0),post('remove',B,9007199254740991,failure('REVISION_CONFLICT'),409),
 get(18,41,.15),get(19,42,.15),get(20,43,.15),
 get(20,44),get(20,45),post('unblock',G,20,None,0),post('unblock',G,20,failure('REVISION_CONFLICT'),409),
 get(21,46),post('unblock',G,20,failure('REVISION_CONFLICT'),409,delay=.15),get(22,47),
]
class Handler(BaseHTTPRequestHandler):
    count=0;failure=None
    def respond(self,status,value):
        payload=json.dumps(value).encode().replace(b'"__ORIGINAL_FRACTION__"',b'9007199254740991.1')
        self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(payload)));self.end_headers()
        try:self.wfile.write(payload)
        except (BrokenPipeError,ConnectionResetError):pass
    def reject(self,message):
        type(self).failure=message;print(message,flush=True);self.respond(422,{'code':'FIXTURE_CONTRACT','message':message})
    def do_GET(self):self.contract('GET')
    def do_POST(self):self.contract('POST')
    def contract(self,method):
        if self.path=='/actuator/health':self.respond(200,{'status':'UP'});return
        body=json.loads(self.rfile.read(int(self.headers.get('Content-Length','0'))) or 'null')
        if self.path=='/api/v1/app/user-management/signin':
            if body!={'type':'ANONYMOUS','anonymousToken':'fixture-anonymous-1009'}:self.reject('wrong original signin');return
            self.respond(200,{'userId':U,'username':'Fixture','accessToken':'fixture-session-1009','authStatus':'AUTHENTICATED','isAnonymous':True,'isVerified':False});return
        index=type(self).count;type(self).count+=1
        if index>=len(EXPECTED):self.reject('unexpected extra request or poll');return
        verb,path,query,payload,status,reply,delay=EXPECTED[index];url=urlparse(self.path)
        if (method,url.path,parse_qs(url.query),body)!=(verb,path,query,payload):self.reject(f'request {index+1} changed original revision/request ID or route');return
        if self.headers.get('X-API-Key')!='fixture-key-1009' or self.headers.get('Authorization')!='Bearer fixture-session-1009':self.reject('wrong original project/session');return
        time.sleep(delay)
        if not status:self.connection.shutdown(socket.SHUT_RDWR);self.connection.close();return
        self.respond(status,copy.deepcopy(reply))
    def log_message(self,*args):pass

def main():
    server=HTTPServer(('127.0.0.1',18815),Handler);server.timeout=.2
    marker=os.environ.get('MOCK_SERVER_READY_FILE')
    if marker:
        with open(marker,'w') as handle:handle.write('ready\n')
    deadline=time.monotonic()+180
    while Handler.count<len(EXPECTED) and not Handler.failure and time.monotonic()<deadline:server.handle_request()
    deadline=time.monotonic()+1.5
    while not Handler.failure and time.monotonic()<deadline:
        server.timeout=max(.01,min(.2,deadline-time.monotonic()));server.handle_request()
    server.server_close()
    if Handler.failure or Handler.count!=len(EXPECTED):raise RuntimeError(f'{Handler.failure}; requests={Handler.count}/{len(EXPECTED)}')
    print(f'Friends strict native contract: {Handler.count} exact feature requests; original revision/IDs and no implicit GET/poll',flush=True)
if __name__=='__main__':main()
