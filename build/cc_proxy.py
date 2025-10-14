import argparse, socket, select, time, json, os, heapq
F_SYN=1; F_ACK=2; F_FIN=4; F_DATA=8
def hdr(b):
    if len(b)<13: return None
    return (b[0], int.from_bytes(b[1:5],'big'), int.from_bytes(b[5:9],'big'))
def now(): return time.time()
class Proxy:
    def __init__(self, fport, bport, log_path, ready_file, drop_seq=0, idle=8.0):
        self.front=socket.socket(socket.AF_INET, socket.SOCK_DGRAM); self.front.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,1); self.front.bind(("127.0.0.1", fport))
        self.back =socket.socket(socket.AF_INET, socket.SOCK_DGRAM); self.back .setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,1); self.back .bind(("127.0.0.1", 0))
        self.server=("127.0.0.1", bport); self.client=None
        self.drop_seq=drop_seq; self.dropped=False
        self.q=[]; self.qid=0; self.idle=idle
        os.makedirs(os.path.dirname(log_path), exist_ok=True)
        self.log=open(log_path,"w"); self.log.flush()
        open(ready_file,"w").close()
    def sched(self,sock,data,dst,dirlab):
        self.qid+=1; heapq.heappush(self.q,(now(),self.qid,sock,data,dst,dirlab))
    def loop(self):
        deadline=now()+self.idle
        while True:
            t=now()
            while self.q and self.q[0][0]<=t:
                _,_,sock,data,dst,dirlab=heapq.heappop(self.q)
                try: sock.sendto(data,dst)
                except: pass
                h=hdr(data)
                if h:
                    f,seq,ack=h
                    rec={"t":now(),"dir":dirlab}
                    if dirlab=="c2s": rec.update({"seq":seq,"flags":f})
                    else: rec.update({"ack":ack,"flags":f})
                    self.log.write(json.dumps(rec)+"\n"); self.log.flush()
                deadline=now()+self.idle
            r,_,_=select.select([self.front,self.back],[],[],0.01)
            for s in r:
                data,src=s.recvfrom(65535)
                if s is self.front:
                    self.client=src
                    h=hdr(data)
                    if h and self.drop_seq>0:
                        f,seq,_=h
                        if (f & F_DATA) and (seq==self.drop_seq) and not self.dropped:
                            self.dropped=True
                            continue
                    self.sched(self.back,data,self.server,"c2s")
                else:
                    if self.client: self.sched(self.front,data,self.client,"s2c")
            if not r and not self.q and now()>deadline: break
        self.log.close()
if __name__=="__main__":
    ap=argparse.ArgumentParser()
    ap.add_argument("--front-port",type=int,required=True)
    ap.add_argument("--back-port", type=int,required=True)
    ap.add_argument("--log",required=True)
    ap.add_argument("--ready-file",required=True)
    ap.add_argument("--drop-seq",type=int,default=0)
    a=ap.parse_args()
    Proxy(a.front_port,a.back_port,a.log,a.ready_file,a.drop_seq).loop()
