#!/usr/bin/env python3
"""Artificial identity recovery fixture. Loopback only; no secrets or real accounts."""
import json,os,re,time
from http.server import BaseHTTPRequestHandler,HTTPServer
from urllib.parse import urlparse,parse_qs
A='00000000-0000-4000-8000-000000001005';E='00000000-0000-4000-8000-000000002005';B='00000000-0000-4000-8000-000000003005';I='00000000-0000-4000-8000-000000004005';J='00000000-0000-4000-8000-000000005005'
PA='identity-issued-1005';PB='identity-foreign-1005';PC='identity-legacy-1005';PU='identity-unknown-1005';PI='identity-inline-1005';PJ='identity-preservation-failed-1005';PK='identity-context-1005'
def user(uid,anonymous=True):return dict(userId=uid,username='Fixture',accessToken='guest-access' if anonymous else 'email-access',isAnonymous=anonymous,isVerified=False,authStatus='AUTHENTICATED')
def snapshot():return dict(userId=A,definitionVersion=1,revision=1,definitions=[dict(key=k,name=k,mode=m,reset='none') for k,m in [('kills','sum'),('best_score','max')]],values=[dict(key=k,value=0,hasValue=False,period=None) for k in ['kills','best_score']],operationWindow=dict(serverTimeUnixMillis=int(time.time()*1000),maxAgeSeconds=86400,maxFutureSkewSeconds=30),newlyUnlocked=[])
class H(BaseHTTPRequestHandler):
    count=0;failure=None
    def log_message(self,*a):pass
    def reply(self,status,data):
        body=json.dumps(data).encode();self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
    def do_GET(self):self.step('GET')
    def do_POST(self):self.step('POST')
    def step(self,method):
        if self.path=='/actuator/health':self.reply(200,{'status':'UP'});return
        H.count+=1;n=H.count
        try:
            p=urlparse(self.path);b=json.loads(self.rfile.read(int(self.headers.get('Content-Length','0'))) or 'null');key=self.headers.get('X-API-Key')
            print(json.dumps(dict(step=n,method=method,path=p.path,project=key)),flush=True)
            assert n<=15,'extra request / replacement signup / premature statistics'
            assert key==({4:PB,5:PB,10:PC,11:PU,12:PU,13:PI,14:PJ,15:PK}.get(n,PA)),'wrong request project'
            if n in [1,4,11,13,14,15]:
                assert method=='POST' and p.path=='/api/v1/app/user-management/signup' and b['type']=='ANONYMOUS'
                if n==11:self.reply(503,{'message':'Unknown signup outcome'})
                else:
                    reply=dict(userId={1:A,4:B,13:I,14:J,15:'00000000-0000-4000-8000-000000006005'}[n],username='Fixture',anonymousToken={1:'issued-token-1005',4:'foreign-token-1005',13:'inline-token',14:'preservation-token',15:'context-token'}[n])
                    if n==13:reply.update(user(I))
                    self.reply(200,reply)
            elif n in [2,3,5,6,10,12]:
                assert method=='POST' and p.path=='/api/v1/app/user-management/signin'
                if n==3:
                    assert b==dict(type='EMAIL',email='fixture@example.invalid',password='synthetic');self.reply(200,user(E,False))
                else:
                    assert b==dict(type='ANONYMOUS',anonymousToken='legacy-original-token-1005' if n in [10,12] else 'foreign-token-1005' if n==5 else 'issued-token-1005')
                    self.reply(200,user(A)) if n==6 else self.reply(401,{'code':'NOT_AUTHENTICATED','message':'Synthetic rejection'})
            elif n==7:
                assert method=='POST' and p.path=='/api/v1/app/user-management/check-auth' and b==dict(userId=A,sessionToken='guest-access')
                time.sleep(.25);self.reply(200,{'isAuthenticated':True})
            elif n==8:
                assert method=='GET' and p.path=='/api/v1/app/statistics' and parse_qs(p.query)==dict(userId=[A]);self.reply(200,snapshot())
            elif n==9:
                assert method=='POST' and p.path=='/api/v1/app/statistics' and b['userId']==A and b['updates']==[dict(key='kills',value=3),dict(key='best_score',value=1200)]
                assert re.fullmatch(r'\d{13}:[0-9a-f-]{36}',b['operationId']);self.reply(410,{'code':'OPERATION_EXPIRED','message':'Keep original batch'})
        except Exception as ex:
            H.failure=f'step {n}: {ex}';print(H.failure,flush=True);self.reply(422,{'code':'FIXTURE_CONTRACT','message':H.failure})
def main():
    s=HTTPServer(('127.0.0.1',18812),H);s.timeout=.1
    if os.environ.get('MOCK_SERVER_READY_FILE'):open(os.environ['MOCK_SERVER_READY_FILE'],'w').write('ready\n')
    deadline=time.monotonic()+100
    while H.count<15 and not H.failure and time.monotonic()<deadline:s.handle_request()
    deadline=time.monotonic()+2
    while time.monotonic()<deadline and not H.failure:s.handle_request()
    s.server_close()
    assert H.count==15 and H.failure is None,(H.count,H.failure)
    print('15 exact requests; no replacement signup or premature statistics',flush=True)
if __name__=='__main__':main()
