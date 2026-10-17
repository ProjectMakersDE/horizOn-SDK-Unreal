#!/usr/bin/env python3
"""Strict two-player artificial Matches HTTP contract. Loopback only; no real accounts."""
import base64,copy,hashlib,json,os,re,socket,threading,time
from decimal import Decimal
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from urllib.parse import urlparse,parse_qs
A='00000000-0000-4000-8000-000000001013';B='00000000-0000-4000-8000-000000002013';C='00000000-0000-4000-8000-000000003013'
ID='10000000-0000-4000-8000-000000001013';OTHER='10000000-0000-4000-8000-000000002013';RUN='30000000-0000-4000-8000-000000001013'
ROOT='/api/v1/app/matches';TIME='2026-10-16T00:00:30Z';LIMITS=dict(activeMatches=2,stateBytes=1024,retainedMatches=20,pendingInvites=20,settlementMatches=100,historyRetentionSeconds=604800,rankedSettlementSeconds=604800)
NAME='a\0雪';ORIGINAL=None
CURSOR=base64.b64encode(b'opaque:strength:1').decode()
def summary(rev=1,status='lobby',participants=None,invited=None,current=None,final=None,**kw):
 return dict(id=ID,name='__NAME__',revision=rev,creatorId=A,capacity=2,visibility='private',strength=-9007199254740991,status=status,participantIds=[A] if participants is None else participants,invitedIds=[B] if invited is None else invited,forfeitedIds=[],currentPlayerId=current,turnDeadline=TIME if current else None,createdAt=TIME,startedAt=TIME if status!='lobby' else None,finishedAt=TIME if final else None,purgeAt=None,finalStateRevision=final,hasDisagreement=status=='disputed',ranked=True,timeoutDue=False,**kw)
def view(rev=1,status='lobby',participants=None,invited=None,current=None,final=None,data='AP8A',reports=None):
 s=summary(rev,status,participants,invited,current,final)
 s.update(turnOrder=[] if status=='lobby' else s['participantIds'],turnSeconds=60,timeoutPolicy='skip',dataBase64=data,reports=reports or [],reportDeadline=TIME if final else None,absoluteDeadline=TIME if status!='lobby' else None,rankedLeaderboardKey=None,ownRankedRunId=None,notificationPending=status in ('reporting','confirmed'))
 return s
def read(v,**kw):return dict(match=v,pollAfterSeconds=120,serverTime=TIME,**kw)
def report(user,result='AP8=',winners=None):return dict(userId=user,finalStateRevision=6,resultBase64=result,winnerIds=[A] if winners is None else winners,reportedAt=TIME)
def ledger(rev=0,active=None,recent=None):return dict(userId=A,revision=rev,active=active or [],invites=[],recent=recent or [],pendingResults=[],limits=LIMITS,pollAfterSeconds=120,serverTime=TIME)
def run(ticket='fixture-match-ticket-2',seconds=599):return dict(runId=RUN,ticket=ticket,seed=42,leaderboardKey=None,issuedAt=TIME,expiresAt='2026-10-16T00:10:30Z',expiresInSeconds=seconds)
INPUT=dict(userId=A,name=NAME,capacity=2,visibility='private',strength=-9007199254740991,turnSeconds=60,timeoutPolicy='skip',ranked=True,dataBase64='AP8A',inviteeIds=[B])
EXPECTED=[]
def add(method,path,reply,*,user=A,body=None,params=None,status=200,rev=None,delay=0,port=18820,race=False):
 EXPECTED.append(dict(method=method,path=ROOT+path,reply=copy.deepcopy(reply),user=user,body=body,params=params or {},status=status,rev=rev,delay=delay,port=port,race=race))
def get(path,reply,user=A,**kw):add('GET',path,reply,user=user,params=dict(userId=[user]),**kw)
def action(name,rev,reply,user=A,body=None,id=ID,**kw):add('POST','/'+id+'/'+name,reply,user=user,body=dict(userId=user,**(body or {})),rev=rev,**kw)
get('/limits',dict(limits=LIMITS,config=dict(revision=1,rankedLeaderboardKey=None),capabilities=dict(matches=True,matchesRanked=True,realtime=True),pollAfterSeconds=120,serverTime=TIME))
openrow=summary();openrow.update(visibility='open',invitedIds=[],name=NAME)
add('GET','/open',dict(matches=[openrow],nextCursor=CURSOR,pollAfterSeconds=120,serverTime=TIME),user=B,params=dict(userId=[B],minStrength=['-9007199254740991'],maxStrength=['9007199254740991'],limit=['20']))
add('GET','/open',dict(matches=[],nextCursor=None,pollAfterSeconds=120,serverTime=TIME),user=B,params=dict(userId=[B],minStrength=['-9007199254740991'],maxStrength=['9007199254740991'],limit=['20'],cursor=[CURSOR]))
get('',dict(code='SERVER_ERROR'),status=503);get('',ledger())
get('',ledger())
add('POST','',None,body='__CREATE__',rev=0,status=0)
# The original project/player pending slot survives region A -> B; only reads follow.
get('',ledger(1,[summary()]),port=18821)
get('/'+ID,read(view()),port=18821)
get('/'+ID,read(view(data=None)),user=B)
action('join',1,read(view(2,participants=[A,B],invited=[])),user=B)
get('/'+ID,read(view(2,participants=[A,B],invited=[])),port=18821)
action('start',2,read(view(3,'active',[A,B],[],A)),body=dict(turnOrder=[A,B]),port=18821)
action('turn',3,dict(code='MATCH_NOT_YOUR_TURN'),user=B,body=dict(dataBase64='Af8=',finish=False),status=409)
get('/'+ID,read(view(3,'active',[A,B],[],A)),port=18821)
action('turn',3,read(view(4,'active',[A,B],[],B,data='AP8AAQ==')),body=dict(dataBase64='AP8AAQ==',finish=False),port=18821)
get('/'+ID,read(view(4,'active',[A,B],[],B,data='AP8AAQ==')),user=B)
action('turn',4,None,user=B,body=dict(dataBase64='AQD/',finish=False),status=0)
action('turn',4,dict(code='MATCH_REVISION_CONFLICT'),user=B,body=dict(dataBase64='AQD/',finish=False),status=409)
get('/'+ID,read(view(5,'active',[A,B],[],A,data='AQD/')),user=B)
get('/'+ID,read(view(5,'active',[A,B],[],A,data='AQD/')),port=18821)
action('turn',5,read(view(6,'reporting',[A,B],[],final=6,data='Av8A')),body=dict(dataBase64='Av8A',finish=True),port=18821)
get('/'+ID,read(view(6,'reporting',[A,B],[],final=6,data='Av8A')),user=B)
action('result',None,read(view(7,'reporting',[A,B],[],final=6,data='Av8A',reports=[report(B)]),replayed=False),user=B,body=dict(finalStateRevision=6,resultBase64='AP8=',winnerIds=[A]))
get('/'+ID,read(view(7,'reporting',[A,B],[],final=6,data='Av8A',reports=[report(B)])),port=18821)
action('result',None,read(view(8,'confirmed',[A,B],[],final=6,data='Av8A',reports=[report(B),report(A)]),replayed=False),body=dict(finalStateRevision=6,resultBase64='AP8=',winnerIds=[A]),port=18821)
action('ranked-run',None,None,body=dict(finalStateRevision=6),status=0,port=18821)
action('ranked-run',None,run(),body=dict(finalStateRevision=6),port=18821)
def submit(ticket,reply,status=200):
 EXPECTED.append(dict(method='POST',path='/api/v1/app/validated-actions/submit',reply=reply,user=A,body=dict(userId=A,ticket=ticket,score=7,stage='match_result_demo',inputLogHash=hashlib.sha256(bytes([1,2,3])).hexdigest()),params={},status=status,rev=None,delay=0,port=18821))
submit('fixture-match-ticket-2',dict(code='RUN_PENDING'),429)
recovered=view(8,'confirmed',[A,B],[],final=6,data='Av8A',reports=[report(B),report(A)]);recovered['ownRankedRunId']=RUN
get('/'+ID,read(recovered),port=18821)
action('ranked-run',None,run('fixture-match-ticket-3',598),body=dict(finalStateRevision=6),port=18821)
submit('fixture-match-ticket-3',dict(runId=OTHER,accepted=True,evidence=dict(required=False)))
action('ranked-run',None,run('fixture-match-ticket-4',597),body=dict(finalStateRevision=6),port=18821)
submit('fixture-match-ticket-4',dict(runId=RUN,accepted=True,sus=False,evidence=dict(required=False)))
action('ranked-run',None,dict(code='RUN_ALREADY_USED'),body=dict(finalStateRevision=6),status=409,port=18821)
# Remaining routes and legal metadata-only departure ACKs.
depart=view(2,data=None);depart.update(id=OTHER,invitedIds=[])
action('decline',1,read(depart),user=B,id=OTHER)
get('/'+OTHER,dict(code='MATCH_NOT_FOUND'),user=B,status=404)
depart['revision']=3
action('leave',2,read(depart),user=B,id=OTHER)
cancel=view(4,'cancelled',invited=[]);cancel['id']=OTHER
action('cancel',3,read(cancel),id=OTHER,port=18821)
skip=view(5,'active',[A,B,C],[],B);skip.update(id=OTHER,capacity=3)
action('timeout',4,read(skip),id=OTHER,port=18821)
forfeit=copy.deepcopy(skip);forfeit.update(revision=6,currentPlayerId=C,forfeitedIds=[A],timeoutPolicy='forfeit')
action('timeout',5,read(forfeit),id=OTHER,port=18821)
conflict=view(9,'disputed',[A,B],[],final=6,reports=[report(A),report(B,'Af8=')]);conflict['id']=OTHER
action('result',None,read(conflict,replayed=False),user=B,id=OTHER,body=dict(finalStateRevision=6,resultBase64='Af8=',winnerIds=[A]))
unconfirmed=view(10,'unconfirmed',[A,B],[],final=6,reports=[report(A)]);unconfirmed['id']=OTHER
get('/'+OTHER,read(unconfirmed),port=18821)
# No mutation transport retry on 503 or code-free/feature 429; explicit unsupported ranked.
add('POST','',dict(code='SERVER_ERROR'),body=INPUT,rev=9007199254740991,status=503,port=18821)
action('turn',9007199254740991,dict(message='capacity'),body=dict(dataBase64='AP8A',finish=False),status=429,port=18821)
action('join',1,dict(code='MATCH_ACTIVE_LIMIT'),status=429,port=18821)
action('ranked-run',None,dict(code='FEATURE_NOT_SUPPORTED'),body=dict(finalStateRevision=6),status=501,port=18821)
get('',ledger(2),port=18821)
add('POST','',dict(code='ACTIVE_MATCH_LIMIT_REACHED'),body='__REJECT_CREATE__',rev=2,status=422,port=18821)
get('',ledger(2),port=18821)
invitation=summary();invitation['id']=OTHER
peerledger=dict(ledger(4),userId=B,invites=[invitation])
get('',peerledger,user=B)
inviteview=view(data=None);inviteview['id']=OTHER
get('/'+OTHER,read(inviteview),user=B)
action('decline',1,None,user=B,id=OTHER,status=0)
action('decline',1,dict(code='MATCH_NOT_FOUND'),user=B,id=OTHER,status=404)
get('',dict(ledger(5),userId=B),user=B)
get('/'+OTHER,dict(code='MATCH_NOT_FOUND'),user=B,status=404)
purged=view();purged['id']=OTHER
get('/'+OTHER,read(purged),port=18821)
action('cancel',1,None,id=OTHER,status=0,port=18821)
get('',ledger(5),port=18821)
get('/'+OTHER,dict(code='MATCH_NOT_FOUND'),status=404,port=18821)
get('/'+ID,read(view(8,'confirmed',[A,B],[],final=6,data='Av8A',reports=[report(B),report(A)])),port=18821)
action('ranked-run',None,dict(code='RANKED_NOT_CONFIGURED'),body=dict(finalStateRevision=6),status=422,port=18821)
get('/'+ID,read(view(8,'confirmed',[A,B],[],final=6,data='Av8A',reports=[report(B),report(A)])),port=18821)
get('',ledger(5),port=18821);get('',ledger(4),port=18821)
old=ledger(5);old['serverTime']='2026-10-16T00:00:29Z';get('',old,port=18821)
bad=ledger(5);bad['revision']='__FRACTION__';get('',bad,port=18821)
for n in range(6,9):get('',ledger(n),delay=.2,port=18821)
get('',ledger(9),port=18821)
get('',ledger(10),delay=.5,port=18821,race=True)
get('/limits',dict(limits=LIMITS,config=dict(revision=1,rankedLeaderboardKey=None),capabilities=dict(matches=True,matchesRanked=True,realtime=True),pollAfterSeconds=120,serverTime=TIME),delay=3,port=18821)
def generic(delay=.3):
 reply=dict(run(),runId='40000000-0000-4000-8000-000000001013',ticket='generic-ticket-1013',leaderboardKey='')
 EXPECTED.append(dict(method='POST',path='/api/v1/app/validated-actions/runs',reply=reply,user=A,body=dict(userId=A),params={},status=200,rev=None,delay=delay,port=18821))
generic()
action('ranked-run',None,run(),body=dict(finalStateRevision=6),delay=.3,port=18821)
action('ranked-run',None,run(),body=dict(finalStateRevision=6),delay=.3,port=18821)
generic()
action('ranked-run',None,run(),body=dict(finalStateRevision=6),delay=.5,port=18821)
action('ranked-run',None,run(),body=dict(finalStateRevision=6),port=18820)
get('/limits',dict(limits=LIMITS,config=dict(revision=1,rankedLeaderboardKey=None),capabilities=dict(matches=True,matchesRanked=True,realtime=True),pollAfterSeconds=120,serverTime=TIME),port=18821)
class Handler(BaseHTTPRequestHandler):
 count=0;failure=None;lock=threading.Lock();original=None
 def respond(self,status,v):
  raw=json.dumps(v,ensure_ascii=False,separators=(',',':')).replace('"__FRACTION__"','9007199254740991.1').encode()
  self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(raw)));self.end_headers()
  try:self.wfile.write(raw)
  except (BrokenPipeError,ConnectionResetError):pass
 def reject(self,why):Handler.failure=why;print(why,flush=True);self.respond(422,dict(code='FIXTURE_CONTRACT',message=why))
 def do_GET(self):self.contract('GET')
 def do_POST(self):self.contract('POST')
 def contract(self,method):
  raw=self.rfile.read(int(self.headers.get('Content-Length','0')))
  try:body=json.loads(raw or 'null',parse_float=Decimal)
  except Exception:self.reject('Invalid outgoing JSON');return
  if self.path=='/actuator/health':self.respond(200,dict(status='UP'));return
  if self.path=='/api/v1/app/user-management/signin':
   if method!='POST' or not body or body.get('type')!='ANONYMOUS' or body.get('anonymousToken') not in ('fixture-anonymous-a-1013','fixture-anonymous-b-1013'):self.reject('No signup/new identity allowed');return
   user=A if body['anonymousToken']=='fixture-anonymous-a-1013' else B
   self.respond(200,dict(userId=user,accessToken='fixture-session-'+('a' if user==A else 'b')+'-1013',username='Fixture',authStatus='AUTHENTICATED',isAnonymous=True,isVerified=False));return
  with Handler.lock:n=Handler.count;Handler.count+=1
  if n>=len(EXPECTED):self.reject('Unexpected retry/poll/new original');return
  e=copy.deepcopy(EXPECTED[n]);url=urlparse(self.path)
  if e['body'] in ('__CREATE__','__REJECT_CREATE__'):
   if not isinstance(body,dict) or not re.fullmatch('horizon-match:[0-9a-fA-F-]{36} a\x00雪',body.get('name','')):self.reject('Durable create nonce/Unicode suffix missing');return
   Handler.original=body['name'];e['body']=dict(INPUT,name=Handler.original)
  def names(v):
   if isinstance(v,dict):return {k:names(x) for k,x in v.items()}
   if isinstance(v,list):return [names(x) for x in v]
   return Handler.original if v=='__NAME__' and Handler.original else (NAME if v=='__NAME__' else v)
  e['reply']=names(e['reply'])
  actual=(method,url.path,parse_qs(url.query),body,self.server.server_port,self.headers.get('If-Match'))
  wanted=(e['method'],e['path'],e['params'],e['body'],e['port'],None if e['rev'] is None else str(e['rev']))
  if actual!=wanted:self.reject(f'Request {n+1} differs: {actual!r}; expected {wanted!r}');return
  token='fixture-session-'+('a' if e['user']==A else 'b')+'-1013'
  if self.headers.get('X-API-Key')!='fixture-key-1013' or self.headers.get('Authorization')!='Bearer '+token:self.reject('Wrong project/player session snapshot');return
  print(f'feature {n+1}: {method} {url.path} origin={self.server.server_port} actor={"A" if e["user"]==A else "B"}',flush=True)
  if e.get('race'):
   with open(os.environ['MOCK_MATCHES_RACE_STARTED_FILE'],'w') as f:f.write('old request received\n')
  time.sleep(e['delay'])
  if not e['status']:
   try:self.connection.shutdown(socket.SHUT_RDWR)
   except OSError:pass
   self.connection.close();return
  self.respond(e['status'],e['reply'])
 def log_message(self,*args):pass
if __name__=='__main__':
 servers=[ThreadingHTTPServer(('127.0.0.1',port),Handler) for port in (18820,18821)]
 for s in servers:threading.Thread(target=s.serve_forever,daemon=True).start()
 marker=os.environ.get('MOCK_SERVER_READY_FILE')
 if marker:
  with open(marker,'w') as f:f.write('ready\n')
 deadline=time.monotonic()+210
 while not Handler.failure and Handler.count<len(EXPECTED) and time.monotonic()<deadline:time.sleep(.05)
 time.sleep(2)
 for s in servers:s.shutdown();s.server_close()
 if Handler.failure or Handler.count!=len(EXPECTED):raise RuntimeError(f'{Handler.failure}; exact requests={Handler.count}/{len(EXPECTED)}')
 print(f'Matches strict native contract: {Handler.count} exact requests; two identities; all14 routes; original durable actions',flush=True)
