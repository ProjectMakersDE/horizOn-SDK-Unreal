#!/usr/bin/env python3
"""Strict artificial Shared Objects contract on two isolated loopback origins.
No real accounts, secrets, Mongo, deploys or external network requests.
"""
import base64,copy,json,os,re,socket,threading,time
from decimal import Decimal
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from urllib.parse import urlparse,parse_qs
USER='00000000-0000-4000-8000-000000001012'
ID='10000000-0000-4000-8000-000000001012'
GROUP='20000000-0000-4000-8000-000000001012'
ROOT='/api/v1/app/shared-objects'
TIME='2026-10-16T00:00:30Z'
LIMITS=dict(collections=1,storageBytes=16384,objectBytes=1024)
SEARCH=dict(a='\0a',b='a\0',c='\0')
ORIGINAL_SEARCH=dict(a='a\0雪',b='a\0',c='\0')
INPUT=dict(userId=USER,dataBase64='AP8A',search=SEARCH,position=dict(x=9007199254740991,y=-9007199254740991))
def usage(rev=0,count=0,serverTime=TIME):return dict(userId=USER,collection='markers',revision=rev,count=count,limits=LIMITS,pollAfterSeconds=120,serverTime=serverTime)
def obj(rev=1,data='AP8A',group=None):return dict(id=ID,collection='markers',ownerId=USER,groupId=group,revision=rev,dataBase64=data,search=SEARCH,position=INPUT['position'],createdAt=TIME,updatedAt=TIME,expiresAt=None)
def query(objects=None,cursor=None):return dict(objects=objects or [],nextCursor=cursor,pollAfterSeconds=120,serverTime=TIME)
def write(rev=1,urev=1,group=None):return dict(object=obj(rev,group=group),usage=usage(urev,1),pollAfterSeconds=120,serverTime=TIME)
def deleted():return dict(objectId=ID,deleted=True,usage=usage(2),pollAfterSeconds=120,serverTime=TIME)
def definitions(key='markers',cursor='markers'):return dict(revision=1,definitions=[dict(key=key,name='雪\0tail',active=False,readPolicy='all',writePolicy='owner',maxObjectBytes=65536,maxObjectsPerOwner=100,ttlSeconds=None,searchFields=[dict(key='power',type='integer')],positionEnabled=True,used=True,revision=1)],nextAfterKey=cursor,limits=LIMITS,serverTime=TIME)
EXPECTED=[]
def add(method,path,reply,*,body=None,params=None,status=200,revision=None,delay=0,port=18818,label=''):
 EXPECTED.append(dict(method=method,path=ROOT+path,body=body,params=params or {},reply=reply,status=status,revision=revision,delay=delay,port=port,label=label))
def getusage(rev,count=0,**kw):add('GET','/markers/usage',usage(rev,count),params=dict(userId=[USER]),**kw)
def q(body=None,reply=None,**kw):add('POST','/markers/query',query() if reply is None else reply,body=dict(userId=USER,mode='list',limit=20) if body is None else dict(userId=USER,limit=20,**body),**kw)
def owner(rev=0,rows=None,cursor=None,requestCursor=None,port=18819):
 getusage(rev,1 if rev else 0,port=port)
 b=dict(userId=USER,mode='owner',ownerId=USER,limit=50)
 if requestCursor:b['cursor']=requestCursor
 add('POST','/markers/query',query(rows,cursor),body=b,port=port)
add('GET','/collections',definitions(),params=dict(userId=[USER],limit=['1']))
add('GET','/collections',definitions('world',None),params=dict(userId=[USER],limit=['1'],afterKey=['markers'],catalogRevision=['1']))
getusage(0)
add('GET','/markers/objects/'+ID,dict(object=obj(),pollAfterSeconds=120,serverTime=TIME),params=dict(userId=[USER]))
q(reply=query([obj()]))
q(dict(mode='owner',ownerId=USER,groupId=GROUP),query([obj(group=GROUP)]))
q(dict(mode='field',field='a',equal='\0a'))
q(dict(mode='field',field='power',min=-9007199254740991,max=9007199254740991))
q(dict(mode='rectangle',minX=-9007199254740991,maxX=9007199254740991,minY=-7,maxY=8))
q(dict(mode='random',field='power',min=1,max=9))
q(dict(mode='list',cursor='opaque_A-1'),query(cursor='opaque_A-2'))
q(status=503,reply=dict(code='SERVER_ERROR'));q()
add('POST','/markers/objects',write(group=GROUP),body=dict(INPUT,groupId=GROUP),revision=0)
add('PUT','/markers/objects/'+ID,write(2),body=INPUT,revision=1)
add('DELETE','/markers/objects/'+ID,deleted(),params=dict(userId=[USER]),revision=2)
for method,path,body,params in [('POST','/markers/objects',INPUT,None),('PUT','/markers/objects/'+ID,INPUT,None),('DELETE','/markers/objects/'+ID,None,dict(userId=[USER]))]:add(method,path,dict(code='SERVER_ERROR'),body=body,params=params,revision=9007199254740991,status=503)
q(status=429,reply=dict(code='COLLECTION_STORAGE_LIMIT'))
add('GET','/collections',dict(code='COLLECTION_REVISION_CONFLICT'),params=dict(userId=[USER],limit=['1'],afterKey=['markers'],catalogRevision=['1']),status=409)
add('GET','/markers/objects/'+ID,dict(code='OBJECT_NOT_FOUND'),params=dict(userId=[USER]),status=404)
getusage(5);getusage(4)
add('GET','/markers/usage',usage(5,serverTime='2026-10-16T00:00:29Z'),params=dict(userId=[USER]))
add('GET','/markers/usage',dict(usage(),revision='__FRACTION__'),params=dict(userId=[USER]))
getusage(6,delay=.2);getusage(7,delay=.2);getusage(8,delay=.2);getusage(9)
getusage(10,delay=.1);getusage(11,delay=1.2)
add('GET','/collections',definitions(cursor=None),params=dict(userId=[USER],limit=['1']))
owner(port=18818)
add('POST','/markers/objects',None,body='__ORIGINAL_CREATE__',revision=0,status=0)
owner(1,cursor='owner_page_2')
owner(1,rows='__ORIGINAL_OBJECT__',requestCursor='owner_page_2')
q(dict(mode='rectangle',minX=-1,maxX=1,minY=-2,maxY=2),port=18819)
q(dict(mode='random',field='power',min=1,max=9),port=18819)
q(dict(mode='rectangle',minX=-1,maxX=1,minY=-2,maxY=2,cursor='rectangle_2'),port=18819)
owner(1,rows='__ORIGINAL_OBJECT__',requestCursor='owner_page_3')
add('PUT','/markers/objects/'+ID,None,body=INPUT,revision=1,status=0,port=18819)
add('PUT','/markers/objects/'+ID,dict(code='REVISION_CONFLICT'),body=INPUT,revision=1,status=409,port=18819)
owner(1,rows=[obj(2)])
owner(1,rows=[obj(2)],requestCursor='owner_page_4')
add('DELETE','/markers/objects/'+ID,dict(code='SERVER_ERROR'),params=dict(userId=[USER]),revision=2,status=503,port=18819)
add('DELETE','/markers/objects/'+ID,deleted(),params=dict(userId=[USER]),revision=2,port=18819)
q(dict(mode='random',field='power',min=2,max=10),reply=query([dict(obj(),ownerId='00000000-0000-4000-8000-000000002012')]),port=18819)
PEER='00000000-0000-4000-8000-000000002012'
DEFENSE_BYTES=f'{ID}:{PEER}:1\n'.encode()+bytes([0,255,0])
import hashlib
EXPECTED.append(dict(method='POST',path='/api/v1/app/validated-actions/runs',body=dict(userId=USER,context=dict(simulationVersion='defense-v1',initialState=base64.b64encode(DEFENSE_BYTES).decode())),params={},reply=dict(runId='30000000-0000-4000-8000-000000001012',ticket='fixture-ticket-1012',seed=42,leaderboardKey='',issuedAt=TIME,expiresAt='2026-10-16T00:10:30Z',expiresInSeconds=600),status=200,revision=None,delay=0,port=18819))
EXPECTED.append(dict(method='POST',path='/api/v1/app/validated-actions/submit',body=dict(userId=USER,ticket='fixture-ticket-1012',score=7,stage='defense_demo',inputLogHash=hashlib.sha256(bytes([1,2,3])).hexdigest()),params={},reply=dict(runId='30000000-0000-4000-8000-000000001012',accepted=True,sus=False,evidence=dict(required=False)),status=200,revision=None,delay=0,port=18819))
class Handler(BaseHTTPRequestHandler):
 count=0;failure=None;original=None;lock=threading.Lock()
 def respond(self,status,value):
  raw=json.dumps(value,ensure_ascii=False,separators=(',',':')).encode().replace(b'"__FRACTION__"',b'9007199254740991.1')
  self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(raw)));self.end_headers()
  try:self.wfile.write(raw)
  except (BrokenPipeError,ConnectionResetError):pass
 def reject(self,why):Handler.failure=why;print(why,flush=True);self.respond(422,dict(code='FIXTURE_CONTRACT',message=why))
 def do_GET(self):self.contract('GET')
 def do_POST(self):self.contract('POST')
 def do_PUT(self):self.contract('PUT')
 def do_DELETE(self):self.contract('DELETE')
 def contract(self,method):
  raw=self.rfile.read(int(self.headers.get('Content-Length','0')))
  try:body=json.loads(raw or 'null',parse_float=Decimal)
  except Exception:self.reject('Invalid outgoing JSON');return
  if self.path=='/actuator/health':self.respond(200,dict(status='UP'));return
  if self.path=='/api/v1/app/user-management/signin':
   if method!='POST' or body!=dict(type='ANONYMOUS',anonymousToken='fixture-anonymous-1012'):self.reject('No signup/new identity allowed');return
   self.respond(200,dict(userId=USER,accessToken='fixture-session-1012',username='Fixture',authStatus='AUTHENTICATED',isAnonymous=True,isVerified=False));return
  with Handler.lock:n=Handler.count;Handler.count+=1
  if n>=len(EXPECTED):self.reject('Unexpected retry, new create, poll or page');return
  e=copy.deepcopy(EXPECTED[n]);url=urlparse(self.path)
  if e['body']=='__ORIGINAL_CREATE__':
   if not isinstance(body,dict) or set(body)!=set(INPUT):self.reject('Original create exact fields');return
   try:data=base64.b64decode(body['dataBase64'],validate=True)
   except Exception:self.reject('Original opaque bytes invalid');return
   if not re.fullmatch(rb'horizon-marker:[0-9a-fA-F-]{36}\n\x00\xff\x00',data):self.reject('Persisted original nonce/binary missing');return
   Handler.original=body['dataBase64'];e['body']=dict(INPUT,dataBase64=Handler.original,search=ORIGINAL_SEARCH)
  if e['reply'] and e['reply'].get('objects')=='__ORIGINAL_OBJECT__':e['reply']['objects']=[dict(obj(data=Handler.original),search=ORIGINAL_SEARCH)]
  actual=(method,url.path,parse_qs(url.query),body,self.server.server_port,self.headers.get('If-Match'))
  wanted=(e['method'],e['path'],e['params'],e['body'],e['port'],None if e['revision'] is None else str(e['revision']))
  if actual!=wanted:self.reject(f'Feature request {n+1} differs from exact original contract: {actual!r} wanted {wanted!r}');return
  if self.headers.get('X-API-Key')!='fixture-key-1012' or self.headers.get('Authorization')!='Bearer fixture-session-1012':self.reject('Wrong captured project/session');return
  print(f'feature {n+1}: {method} {url.path} origin={self.server.server_port}',flush=True)
  time.sleep(e['delay'])
  if not e['status']:
   try:self.connection.shutdown(socket.SHUT_RDWR)
   except OSError:pass
   self.connection.close();return
  self.respond(e['status'],e['reply'])
 def log_message(self,*args):pass
if __name__=='__main__':
 servers=[ThreadingHTTPServer(('127.0.0.1',port),Handler) for port in (18818,18819)]
 threads=[threading.Thread(target=s.serve_forever,daemon=True) for s in servers]
 for t in threads:t.start()
 marker=os.environ.get('MOCK_SERVER_READY_FILE')
 if marker:
  with open(marker,'w') as f:f.write('ready\n')
 deadline=time.monotonic()+180
 while not Handler.failure and Handler.count<len(EXPECTED) and time.monotonic()<deadline:time.sleep(.05)
 time.sleep(2)
 for s in servers:s.shutdown();s.server_close()
 if Handler.failure or Handler.count!=len(EXPECTED):raise RuntimeError(f'{Handler.failure}; exact requests={Handler.count}/{len(EXPECTED)}')
 print(f'Shared Objects strict native contract: {Handler.count} exact feature requests; host-switch original retained',flush=True)
