#!/usr/bin/env python3
"""Strict artificial Achievement/native contract; isolated 127.0.0.1:18819 only."""
import copy,hashlib,json,os,socket,time
from http.server import BaseHTTPRequestHandler,HTTPServer
from urllib.parse import parse_qs,urlparse
USER='00000000-0000-4000-8000-000000001008'
ENTRY='message:00000000-0000-4000-8000-000000001008'
SECOND='message:00000000-0000-4000-8000-000000002008'
ROOT='/api/v1/app/achievements'
def version(label):return hashlib.sha256(label.encode()).hexdigest()
def definition(key='welcome',amount=7,mode='manual',hidden=True):
    return {'key':key,'name':key,'description':'fixture','imageUrl':None,'hidden':hidden,'active':True,'mode':mode,
            'condition':None if mode=='manual' else {'statisticKey':'wins','threshold':3},'rewards':{'values':[{'key':'gold','amount':amount}]}}
def progress(key='welcome',state='pending',amount=7):
    return {'key':key,'unlockedAt':None if state=='complete' else '2026-10-16T00:00:01Z','rewardState':state,
            'inboxEntryId':(ENTRY if key=='welcome' else SECOND) if state=='delivered' else None,
            'pendingReward':{'issuedAt':'2026-10-16T00:00:01Z','values':[{'key':'gold','amount':amount}]} if state=='pending' else None}
def snapshot(label='A',revision=0,second=1,definitions=True,empty=False,rewards=7,states=()):
    rows=[definition('statistic',mode='statistic',hidden=False)]
    if states:rows += [definition(amount=rewards),definition('second',5,hidden=False)]
    return {'userId':USER,'revision':revision,'definitionVersion':version(label),'definitions':None if not definitions else [] if empty else rows,
            'progress':[progress(key,state,7 if key=='welcome' else 5) for key,state in states],
            'limits':{'maxDefinitions':20,'maxLifetimeKeys':500},'serverTime':f'2026-10-16T00:00:{second:02d}Z'}
def result(key,state,code=None):return {'key':key,'state':state,'inboxEntryId':(ENTRY if key=='welcome' else SECOND) if state=='delivered' else None,'code':code}
PENDING=(('welcome','pending'),)
PARTIAL=(('welcome','delivered'),('second','pending'))
DELIVERED=(('welcome','delivered'),('second','delivered'))
def get(reply,known=None,delay=0):return ('GET',ROOT,{'userId':[USER],**({'definitionVersion':[version(known)]} if known else {})},None,200,reply,delay)
def post(action,body,reply,status=200,delay=0):return ('POST',ROOT+action,{},dict(userId=USER,**body),status,reply,delay)
def unlock(reply,replayed=False):return {'achievements':reply,'newlyUnlocked':[] if replayed else ['welcome'],'replayed':replayed,'deliveryResults':[result('welcome','pending','INBOX_FULL')]}
bad_revision=snapshot('J',8,23,states=DELIVERED);bad_revision['revision']='__ORIGINAL_UNSAFE_INTEGER__'
bad_hidden=snapshot('J',8,23,states=DELIVERED);bad_hidden['definitions'][0]['hidden']='true'
EXPECTED=[
 get(snapshot()),get(snapshot(definitions=False),known='A'),
 get(snapshot(definitions=False),known='A'),get(snapshot()),
 get(snapshot('B',second=4,empty=True),known='A'),get(snapshot('B',second=5,definitions=False),known='B'),
 post('/unlock',{'key':'welcome'},unlock(snapshot('C',1,6,states=PENDING))),
 post('/unlock',{'key':'welcome'},None,status=0),
 post('/unlock',{'key':'welcome'},unlock(snapshot('D',1,7,rewards=99,states=PENDING),True)),
 post('/rewards/deliver',{'keys':['welcome','second']},{'achievements':snapshot('E',3,9,rewards=99,states=PARTIAL),'results':[result('welcome','delivered'),result('second','pending','INBOX_FULL')]}),
 post('/rewards/deliver',{'keys':['welcome','second']},{'achievements':snapshot('F',4,10,rewards=99,states=DELIVERED),'results':[result('welcome','delivered'),result('second','delivered')]}),
 get(snapshot('E',3,11,states=PARTIAL),known='F'),
 get(snapshot('E',2,11,states=PARTIAL)),get(snapshot('F',4,12,rewards=99,states=DELIVERED)),
 get(snapshot('G',4,5,states=DELIVERED),known='F'),
 get(snapshot('G',5,14,states=DELIVERED),known='F'),
 get(snapshot('G',5,15,definitions=False,states=DELIVERED),known='G',delay=.15),
 get(snapshot('G',5,15,definitions=False,states=DELIVERED),known='G'),
 get(snapshot('G',5,15,definitions=False,states=DELIVERED)),
 get(snapshot('G',5,16,states=DELIVERED)),
 get(snapshot('H',6,17,states=DELIVERED),known='G',delay=.15),
 get(snapshot('H',6,18,states=DELIVERED)),
 get(snapshot('I',7,19,states=DELIVERED),known='H',delay=.15),
 get(snapshot('I',7,20,states=DELIVERED)),
 get(snapshot('J',8,21,states=DELIVERED),known='I',delay=.15),
 get(snapshot('J',8,22,states=DELIVERED)),
 get(bad_revision,known='J'),get(bad_hidden,known='J'),
 post('/unlock',{'key':'statistic'},{'code':'ACHIEVEMENT_NOT_MANUAL','message':'configured statistic mode'},404),
 post('/rewards/deliver',{'keys':['welcome','second']},{'code':'STATE_CONFLICT','message':'retry original keys'},409),
 get(snapshot('K',9,24,states=DELIVERED),known='J',delay=.15),
 get(snapshot('L',10,25,states=DELIVERED),known='K',delay=.15),
 get(snapshot('M',11,26,states=DELIVERED)),
]
compact=snapshot('N',12,27,states=(('welcome','complete'),))
for replay in (False,True):
    EXPECTED.append(post('/unlock',{'key':'welcome'},{'achievements':compact,'newlyUnlocked':[] if replay else ['welcome'],'replayed':replay,'deliveryResults':[]}))
large=snapshot('O',13,28)
large['limits']={'maxDefinitions':800,'maxLifetimeKeys':1000}
keys=['welcome']+[f'a{i}' for i in range(1,701)]
large['definitions']=[definition(key) for key in keys]
large['progress']=[progress(key,'complete') for key in keys]
EXPECTED.append(get(large,known='N'))
lowered=copy.deepcopy(large);lowered['limits']={'maxDefinitions':20,'maxLifetimeKeys':50};lowered['serverTime']='2026-10-16T00:00:29Z'
EXPECTED.append(get(lowered,known='O'))
EXPECTED.append(('POST','/api/v1/app/statistics',{}, {'userId':USER,'operationId':'1792195200000:00000000-0000-4000-8000-000000001008','updates':[{'key':'wins','value':1}]},200,{},.15))
class Handler(BaseHTTPRequestHandler):
    count=0;failure=None
    def json(self,status,value):
        payload=json.dumps(value).encode().replace(b'"__ORIGINAL_UNSAFE_INTEGER__"',b'9007199254740991.1');self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(payload)));self.end_headers();self.wfile.write(payload)
    def reject(self,message):
        type(self).failure=message;print(message,flush=True);self.json(422,{'code':'FIXTURE_CONTRACT','message':message})
    def do_GET(self):self.handle_contract('GET')
    def do_POST(self):self.handle_contract('POST')
    def handle_contract(self,method):
        if self.path=='/actuator/health':self.json(200,{'status':'UP'});return
        body=json.loads(self.rfile.read(int(self.headers.get('Content-Length','0'))) or 'null')
        if self.path=='/api/v1/app/user-management/signin':
            if body!={'type':'ANONYMOUS','anonymousToken':'fixture-anonymous-1008'}:self.reject('wrong original signin');return
            self.json(200,{'userId':USER,'username':'Fixture','accessToken':'fixture-session-1008','authStatus':'AUTHENTICATED','isAnonymous':True,'isVerified':False});return
        index=type(self).count;type(self).count+=1
        if index>=len(EXPECTED):self.reject('unexpected extra request/poll');return
        verb,path,query,payload,status,reply,delay=EXPECTED[index];url=urlparse(self.path)
        if (method,url.path,parse_qs(url.query),body)!=(verb,path,query,payload):self.reject(f'request {index+1} wrong route/version/original keys');return
        if self.headers.get('X-API-Key')!='fixture-key-1008' or self.headers.get('Authorization')!='Bearer fixture-session-1008':self.reject('wrong original project/session');return
        time.sleep(delay)
        if not status:self.connection.shutdown(socket.SHUT_RDWR);self.connection.close();return
        self.json(status,copy.deepcopy(reply))
    def log_message(self,*args):pass

def main():
    server=HTTPServer(('127.0.0.1',18819),Handler);server.timeout=.2
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
    print(f'Achievements strict native contract: {Handler.count} exact requests; no extra retries/polling',flush=True)
if __name__=='__main__':main()
