#!/usr/bin/env python3
"""Artificial loopback-only GiftCode/PV regression. No live accounts or secrets."""
import copy,json,os,socket,time
from http.server import BaseHTTPRequestHandler,HTTPServer
USER='00000000-0000-4000-8000-000000001007'
ROOT='/api/v1/app/gift-codes/redeem'

def receipt(revision=30,replayed=False):
    return {'success':True,'message':'accepted','giftData':'{"original":"opaque"}',
            'grantedUnlocks':['fixture-hat'],'grantedValues':[{'key':'gold','amount':7}],
            'playerValues':{'userId':USER,'definitionVersion':1,'revision':revision,'day':'2026-10-16',
                'values':[{'key':'gold','balance':100,'earnedToday':0,'dailyCap':None}],
                'operationWindow':{'serverTime':f'2026-10-16T00:00:{revision:02d}Z','oldestAcceptedAt':'2026-09-16T00:00:00Z'}},'replayed':replayed}
def case(code,value,status=200,delay=0,id=None):
    return ({'code':code,'userId':USER,**({'idempotencyId':id} if id is not None else {})},status,value,delay)
invalid_fraction=receipt();invalid_fraction['grantedValues'][0]['amount']=7.1
wrong_user=receipt();wrong_user['playerValues']['userId']='00000000-0000-4000-8000-000000009999'
missing_state=receipt();missing_state.pop('playerValues')
invalid_replay=receipt();invalid_replay['replayed']='true'
EXPECTED=[
    case('LEGACY',{'success':True,'message':'old backend','giftData':'{"untouched":true}'}),
    case('VALUES',None,0,id='original-1007'), # Committed receipt, response lost.
    case('VALUES',receipt(replayed=True),id='original-1007'),
    case('VALUES',receipt(replayed=True),id='original-1007'),
    case('LEGACY-503',{'code':'UNAVAILABLE','message':'uncertain legacy result'},503),
    case('MAX',{'code':'PV_MAX_BALANCE','message':'atomic rejection'},409,id='max-1007'),
    case('FRACTION',invalid_fraction,id='fraction-1007'),
    case('WRONG-USER',wrong_user,id='wrong-user-1007'),
    case('OLD',receipt(29),id='old-1007'),
    case('PV-CLEAR',receipt(31),delay=.15,id='pv-clear-1007'),
    case('BASELINE',receipt(31),id='baseline-1007'),
    case('GIFT-CLEAR',receipt(32),delay=.15,id='gift-clear-1007'),
    case('SESSION-ABA',receipt(32),delay=.15,id='session-aba-1007'),
    case('PROJECT-ABA',receipt(32),delay=.15,id='project-aba-1007'),
    case('SIGNOUT',receipt(32),delay=.15,id='signout-1007'),
    case('RESTORED',receipt(32),id='restored-1007'),
    case('OLD-TYPED',{'success':True,'message':'old backend','giftData':'null'},id='old-typed-1007'),
    case('NULL',{'success':True,'message':'no opaque payload','giftData':None}),
    case('REJECTED',{'success':False,'message':'not accepted','giftData':'unchanged'},id='rejected-1007'),
    case('BAD-REPLAY',invalid_replay,id='bad-replay-1007'),
    case('NO-STATE',missing_state,id='no-state-1007'),
    case('BUSY',receipt(33),delay=.15,id='busy-1007'),
]
class Handler(BaseHTTPRequestHandler):
    count=0;failure=None
    def json(self,status,value):
        payload=json.dumps(value,ensure_ascii=False).encode();self.send_response(status)
        self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(payload)));self.end_headers();self.wfile.write(payload)
    def do_GET(self):
        if self.path=='/actuator/health':self.json(200,{'status':'UP'})
        else:self.reject('unexpected GET')
    def reject(self,message):
        type(self).failure=message;print(message,flush=True);self.json(422,{'code':'FIXTURE_CONTRACT','message':message})
    def do_POST(self):
        body=json.loads(self.rfile.read(int(self.headers.get('Content-Length','0'))) or 'null')
        if self.path=='/api/v1/app/user-management/signin':
            if body!={'type':'ANONYMOUS','anonymousToken':'fixture-anonymous-1007'}:self.reject('wrong original identity');return
            self.json(200,{'userId':USER,'username':'Fixture','accessToken':'fixture-session-1007','authStatus':'AUTHENTICATED','isAnonymous':True,'isVerified':False});return
        index=type(self).count;type(self).count+=1
        if index>=len(EXPECTED):self.reject('unexpected extra request');return
        expected,status,value,delay=EXPECTED[index]
        if self.path!=ROOT or body!=expected:self.reject(f'request {index+1} changed route, code, user or original ID');return
        if self.headers.get('X-API-Key')!='fixture-key-1007' or self.headers.get('Authorization')!='Bearer fixture-session-1007':self.reject('wrong project/session');return
        time.sleep(delay)
        if not status:self.connection.shutdown(socket.SHUT_RDWR);self.connection.close();return
        self.json(status,copy.deepcopy(value))
    def log_message(self,*args):pass

def main():
    server=HTTPServer(('127.0.0.1',18818),Handler);server.timeout=.2
    marker=os.environ.get('MOCK_SERVER_READY_FILE')
    if marker:
        with open(marker,'w') as f:f.write('ready\n')
    deadline=time.monotonic()+180
    while Handler.count<len(EXPECTED) and not Handler.failure and time.monotonic()<deadline:server.handle_request()
    deadline=time.monotonic()+1.5
    while not Handler.failure and time.monotonic()<deadline:
        server.timeout=max(.01,min(.2,deadline-time.monotonic()));server.handle_request()
    server.server_close()
    if Handler.failure or Handler.count!=len(EXPECTED):raise RuntimeError(f'{Handler.failure}; requests={Handler.count}/{len(EXPECTED)}')
    print(f'Gift values contract: {Handler.count} exact redeem requests; original identity/payload, lost-response replay and no extra legacy retry passed',flush=True)
if __name__=='__main__':main()
