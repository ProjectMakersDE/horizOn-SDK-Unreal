#!/usr/bin/env python3
"""Artificial HTTP+RFC6455 chat fixture, loopback only. No database or real credentials."""
import base64,copy,datetime,fcntl,hashlib,json,os,re,socket,socketserver,struct,threading,time,uuid
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from urllib.parse import urlparse,parse_qs
A='00000000-0000-4000-8000-000000001014';B='00000000-0000-4000-8000-000000002014';ID='10000000-0000-4000-8000-000000001014';GROUP='10000000-0000-4000-8000-000000002014';ROOT='/api/v1/app/chat'
def stamp(s=0):return (datetime.datetime.now(datetime.timezone.utc)+datetime.timedelta(seconds=s)).isoformat(timespec='milliseconds').replace('+00:00','Z')
NOW=stamp();LATER=(datetime.datetime.fromisoformat(NOW.replace("Z","+00:00"))+datetime.timedelta(seconds=604800)).isoformat(timespec="milliseconds").replace("+00:00","Z");MOD=dict(muteUntil=None,banUntil=None)
LIMITS=dict(retainedMessagesPerPlayer=0,retainedMessagesPerChannel=20,retentionSeconds=604800,privateRetainedMessagesPerChannel=15,privateRetentionSeconds=30,operationWindowSeconds=30,operationFutureToleranceSeconds=60,messageCharacters=500,messageUtf8Bytes=2000,channelsPerAccount=2000,sendsPerAccountMinute=120,playerSendIntervalMillis=3000,channelSendIntervalMillis=2000,reportsPerProject=200,receiptCount=0,reportReceiptCount=0,wordFilterTerms=100)
class State:
 lock=threading.RLock();sockets=[];failure='';requests=[];syncs=0;active=0;peak=0;rows=[];rev=1;epoch='20000000-0000-4000-8000-000000001014';permission=True;mode='normal';receipts={};originals={};counts={};reports=[];done=False;lockfile=None;retry=None;unsupported=False
 @classmethod
 def fail(c,m):c.failure=m;print('FAIL '+m,flush=True)
 @classmethod
 def channel(c):return dict(id=ID,epoch=c.epoch,kind='project',peerIds=[],groupId=None,revision=c.rev,expiresAt=LATER)
 @classmethod
 def push(c,kind='event',reason=None):
  f=dict(type=kind,version=1,topic='chat:'+ID,eventId=uuid.uuid4().hex)
  if reason:f['reason']=reason
  for w in list(c.sockets):
   if w.authorized or kind=='resync_required':
    try:w.send(f)
    except OSError:pass
class WS(socketserver.BaseRequestHandler):
 def exact(self,n):
  b=b''
  while len(b)<n:
   p=self.request.recv(n-len(b))
   if not p:raise EOFError
   b+=p
  return b
 def frame(self):
  a,b=self.exact(2);n=b&127
  if n==126:n=struct.unpack('!H',self.exact(2))[0]
  elif n==127:n=struct.unpack('!Q',self.exact(8))[0]
  assert b&128 and n<=8192
  mask=self.exact(4);p=self.exact(n)
  if a&15==8:raise EOFError
  return json.loads(bytes(c^mask[i%4] for i,c in enumerate(p)))
 def send(self,j):
  p=json.dumps(j,separators=(',',':')).encode();h=bytes([129,len(p)]) if len(p)<126 else bytes([129,126])+struct.pack('!H',len(p))
  with self.send_lock:self.request.sendall(h+p)
 def handle(self):
  self.send_lock=threading.Lock();self.authorized=False
  try:
   h=b''
   while b'\r\n\r\n' not in h:h+=self.exact(1)
   key=re.search(br'Sec-WebSocket-Key:\s*(\S+)',h,re.I).group(1);accept=base64.b64encode(hashlib.sha1(key+b'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').digest())
   self.request.sendall(b'HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: '+accept+b'\r\n\r\n')
   f=self.frame();assert f['type']=='authenticate' and f['version']==1 and f['apiKey']=='fixture-key-1014' and f['sessionToken']=='fixture-session-1014'
   State.sockets.append(self);self.send(dict(type='ready',version=1,pollAfterSeconds=120))
   while not State.done:
    f=self.frame();t=f['type']
    if t=='subscribe':
     assert f['topic']=='chat:'+ID
     if not State.permission:self.send(dict(type='error',version=1,code='SUBSCRIPTION_FORBIDDEN',topic=f['topic'],requestId=f['requestId']));continue
     self.authorized=True;self.send(dict(type='subscribed',version=1,topic=f['topic'],requestId=f['requestId']));self.send(dict(type='resync_required',version=1,topic=f['topic'],reason='SUBSCRIBED',eventId=uuid.uuid4().hex))
    elif t=='unsubscribe':self.authorized=False;self.send(dict(type='unsubscribed',version=1,topic=f['topic'],requestId=f.get('requestId',0)))
    elif t=='ping':self.send(dict(type='pong',version=1))
    else:assert t=='ack'
  except (EOFError,OSError):pass
  except Exception as e:State.fail('WS '+repr(e))
  finally:
   if self in State.sockets:State.sockets.remove(self)
class WebServer(socketserver.ThreadingTCPServer):allow_reuse_address=True;daemon_threads=True
class HTTP(BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def log_message(self,*a):pass
 def reply(self,s,j):
  b=json.dumps(j,ensure_ascii=False,separators=(',',':')).encode();self.send_response(s);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(b)));self.send_header('Connection','close');self.send_header('Retry-After','1');self.end_headers()
  try:self.wfile.write(b)
  except OSError:pass
 def do_GET(self):self.call('GET')
 def do_POST(self):self.call('POST')
 def call(self,v):
  try:self.checked(v)
  except Exception as e:State.fail(repr(e));self.reply(422,dict(code='FIXTURE_CONTRACT',message=str(e)))
 def checked(self,verb):
  u=urlparse(self.path);p=u.path;q=parse_qs(u.query);raw=self.rfile.read(int(self.headers.get('Content-Length','0')));b=json.loads(raw) if raw else None
  if p=='/actuator/health':self.reply(200,dict(status='UP'));return
  if p=='/api/v1/app/user-management/signin':
   assert verb=='POST' and b['anonymousToken']=='fixture-anonymous-1014' and b['type']=='ANONYMOUS';self.reply(200,dict(userId=A,accessToken='fixture-session-1014',username='Fixture',authStatus='AUTHENTICATED',isAnonymous=True,isVerified=False));return
  if p.startswith('/fixture/'):
   n=p[9:]
   with State.lock:
    if n=='populate':State.rows=[dict(id=str(uuid.UUID(int=i+101400)),channelId=ID,authorId=A,text='snow 雪 😀\n\t '+str(i),createdAt=NOW,expiresAt=LATER) for i in range(20)];State.rev+=1;State.push()
    elif n=='delete':State.rows=State.rows[1:];State.rev+=1;State.push()
    elif n=='burst':
     State.mode='burst'
     for _ in range(25):State.push()
    elif n=='permission':
     State.permission=False
     for w in State.sockets:w.authorized=False
     State.push('resync_required','PERMISSIONS_CHANGED')
    elif n=='allow':State.permission=True
    elif n=='disconnect':
     for w in list(State.sockets):
      try:w.request.shutdown(socket.SHUT_RDWR)
      except OSError:pass
    elif n=='epoch':State.epoch=str(uuid.uuid4());State.rows=[];State.rev=1;State.push()
    elif n=='ban-send':State.mode=n
    elif n in ('invalid','delay','read-retry'):State.mode=n;State.push()
    elif n=='lock':
     assert b and re.fullmatch(r'horizon-chat-[0-9a-f]{64}',b['name']);path=Path(b['path']);assert path.name==b['name'] and path.parent.is_dir();State.lockfile=path.open('a');fcntl.flock(State.lockfile,fcntl.LOCK_EX|fcntl.LOCK_NB)
    elif n=='unlock':
     if State.lockfile:fcntl.flock(State.lockfile,fcntl.LOCK_UN);State.lockfile.close();State.lockfile=None
    elif n=='unsupported':State.unsupported=True
    elif n=='done':assert State.peak<=1 and len(State.receipts)>=2 and len(State.reports)>=2 and all(n<=2 for n in State.counts.values());State.done=True
    else:raise ValueError('unknown fixture '+n)
   self.reply(200,dict(ok=True,syncs=State.syncs,peak=State.peak));return
  assert self.headers.get('X-API-Key')=='fixture-key-1014' and self.headers.get('Authorization')=='Bearer fixture-session-1014' and not self.headers.get('If-Match')
  if p=='/api/v1/app/realtime/session':self.reply(200,dict(pollAfterSeconds=120));return
  assert p.startswith(ROOT);assert ((b or {}).get('userId') if verb=='POST' else q.get('userId',[None])[0])==A
  with State.lock:State.requests.append((verb,p,self.server.server_port));print('feature '+str(len(State.requests))+': '+verb+' '+p,flush=True)
  if State.unsupported:self.reply(501,dict(code='NOT_SUPPORTED'));return
  if p==ROOT+'/limits':self.reply(200,dict(limits=LIMITS,capabilities=dict(chat=True,realtime=True,incrementalSync=True,localHistory=True),moderation=MOD,serverTime=stamp()));return
  if p==ROOT+'/channels/open':
   assert set(b) in ({'userId','kind'},{'userId','kind','peerId'},{'userId','kind','groupId'});c=State.channel()
   if b['kind']=='direct':assert b['peerId']==B;c.update(kind='direct',peerIds=[A,B])
   elif b['kind']=='group':assert b['groupId']==GROUP;c.update(kind='group',groupId=GROUP)
   else:assert b['kind']=='project'
   self.reply(200,dict(channel=c,serverTime=stamp()));return
  if p==ROOT+'/channels/'+ID+'/sync':
   assert set(b)=={'userId','epoch','knownMessageIds'} and len(b['knownMessageIds'])<=20 and len(set(b['knownMessageIds']))==len(b['knownMessageIds'])
   with State.lock:
    State.syncs+=1;State.active+=1;State.peak=max(State.peak,State.active);assert State.active==1;mode=State.mode;State.mode='normal';c=State.channel();rows=copy.deepcopy(State.rows);reply=dict(channel=c,visibleMessageIds=[m['id'] for m in rows],messages=[m for m in rows if b['epoch']!=c['epoch'] or m['id'] not in b['knownMessageIds']],moderation=MOD,serverTime=stamp())
   if mode=='burst':time.sleep(.3)
   if mode=='delay':Path(os.environ['MOCK_CHAT_DELAY_MARKER']).write_text('received');time.sleep(.7)
   if mode=='read-retry':
    State.retry=raw
    with State.lock:State.active-=1
    self.reply(503,dict(code='SERVER_ERROR'));return
   if State.retry is not None:assert raw==State.retry;State.retry=None
   if mode=='invalid' and rows:reply['visibleMessageIds'].append(str(uuid.uuid4()))
   with State.lock:State.active-=1
   self.reply(200,reply);return
  m=re.fullmatch(ROOT+r'/channels/([^/]+)/messages',p)
  if m:
   assert set(b)=={'userId','operationId','issuedAt','text'} and m[1]==ID;op=b['operationId'];assert len(op)==36 and len(b['text'])<=500 and len(b['text'].encode())<=2000
   with State.lock:
    State.counts[op]=State.counts.get(op,0)+1;n=State.counts[op]
    if op in State.originals:assert b==State.originals[op]
    else:State.originals[op]=copy.deepcopy(b)
    if State.mode=='ban-send':State.mode='normal';self.reply(403,dict(code='CHAT_BANNED'));return
    if b['text'].startswith('retry') and n==1:self.reply(503,dict(code='SERVER_ERROR'));return
    if b['text'].startswith('expired'):self.reply(409,dict(code='OPERATION_EXPIRED'));return
    if b['text'].startswith('rate'):self.reply(429,dict(code='CHAT_RATE_LIMITED'));return
    replay=op in State.receipts
    if not replay:
     msg=dict(id=str(uuid.uuid4()),channelId=ID,authorId=A,text=b['text'],createdAt=stamp(),expiresAt=LATER);State.rows.append(msg);State.rev+=1;State.receipts[op]=dict(operationId=op,issuedAt=b['issuedAt'],channelId=ID,messageId=msg['id'],createdAt=msg['createdAt'],expiresAt=LATER);State.push()
    receipt=copy.deepcopy(State.receipts[op])
   if b['text'].startswith('lost') and n==1:self.connection.shutdown(socket.SHUT_RDWR);self.connection.close();return
   self.reply(200 if replay else 201,dict(receipt=receipt,replayed=replay,serverTime=stamp()));return
  m=re.fullmatch(ROOT+r'/operations/([^/]+)',p)
  if m:
   receipt=State.receipts.get(m[1]);self.reply(200,dict(receipt=receipt,serverTime=stamp())) if receipt else self.reply(404,dict(code='OPERATION_NOT_FOUND'));return
  m=re.fullmatch(ROOT+r'/channels/([^/]+)/messages/([^/]+)/report',p)
  if m:
   assert m[1]==ID and set(b)=={'userId','reason'};State.reports.append((m[1],m[2],copy.deepcopy(b)));assert all(x==State.reports[0] for x in State.reports)
   if len(State.reports)==1:self.reply(503,dict(code='SERVER_ERROR'));return
   self.reply(200,dict(reportId='30000000-0000-4000-8000-000000001014',status='removed',replayed=True,serverTime=stamp()));return
  raise ValueError('unexpected route/retry '+p)
if __name__=='__main__':
 servers=[ThreadingHTTPServer(('127.0.0.1',p),HTTP) for p in (18822,18823)]+[WebServer(('127.0.0.1',18824),WS)]
 for s in servers:threading.Thread(target=s.serve_forever,daemon=True).start()
 Path(os.environ['MOCK_SERVER_READY_FILE']).write_text('ready');deadline=time.monotonic()+210
 while not State.done and not State.failure and time.monotonic()<deadline:time.sleep(.05)
 time.sleep(1)
 for s in servers:s.shutdown();s.server_close()
 if State.lockfile:State.lockfile.close()
 if State.failure or not State.done:raise RuntimeError(State.failure or 'fixture timeout')
 print(json.dumps(dict(featureRequests=len(State.requests),syncRequests=State.syncs,maxConcurrentSync=State.peak,sendAttempts=sorted(State.counts.values()),reports=len(State.reports))),flush=True)
