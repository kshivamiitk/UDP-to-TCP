import argparse, socket, select, time, random, heapq, json, sys, os
F_SYN=1; F_ACK=2; F_FIN=4; F_DATA=8
def parse_hdr(b):
    if len(b)<13: return None
    return (b[0], int.from_bytes(b[1:5],'big'), int.from_bytes(b[5:9],'big'))
def now(): return time.time()
class Proxy:
    def __init__(self, fport, back_host, back_port, loss, delay_ms, jitter_ms, reorder_pct, reorder_ms, log_path, idle_s, ready_file):
        self.front=socket.socket(socket.AF_INET, socket.SOCK_DGRAM); self.front.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); self.front.bind(("127.0.0.1", fport))
        self.back=socket.socket(socket.AF_INET, socket.SOCK_DGRAM); self.back.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); self.back.bind(("127.0.0.1", 0))
        self.server=(back_host, back_port); self.client=None
        self.loss=loss; self.delay=delay_ms/1000.0; self.jitter=jitter_ms/1000.0; self.reorder=reorder_pct/100.0; self.ro_ms=reorder_ms/1000.0
        self.q=[]; self.qid=0; self.log_path=log_path; self.idle_s=idle_s; self.ready_file=ready_file
        os.makedirs(os.path.dirname(log_path), exist_ok=True)
        self.log=open(log_path,"w"); self.log.flush()
        open(self.ready_file, "w").close()
    def sched(self, sock, data, dst, dirlab):
        if random.random()<self.loss: return
        d=self.delay + (random.random()*2-1)*self.jitter
        if random.random()<self.reorder: d+=self.ro_ms
        due=now()+max(0.0,d); self.qid+=1
        heapq.heappush(self.q,(due,self.qid,sock,data,dst,dirlab))
    def loop(self):
        deadline=now()+self.idle_s
        while True:
            t=now()
            while self.q and self.q[0][0]<=t:
                _,_,sock,data,dst,dirlab=heapq.heappop(self.q)
                try: sock.sendto(data,dst)
                except: pass
                h=parse_hdr(data)
                if h:
                    f,seq,ack=h
                    rec={"t":now(),"dir":dirlab}
                    if dirlab=="c2s": rec.update({"seq":seq,"flags":f})
                    else: rec.update({"ack":ack,"flags":f})
                    self.log.write(json.dumps(rec)+"\n"); self.log.flush()
                deadline=now()+self.idle_s
            r,_,_=select.select([self.front,self.back],[],[],0.01)
            for s in r:
                data,src=s.recvfrom(65535)
                if s is self.front:
                    self.client=src; self.sched(self.back,data,self.server,"c2s")
                else:
                    if self.client: self.sched(self.front,data,self.client,"s2c")
            if not r and not self.q and now()>deadline: break
        self.log.close()
if __name__=="__main__":
    ap=argparse.ArgumentParser()
    ap.add_argument("--front-port", type=int, required=True)
    ap.add_argument("--back-port", type=int, required=True)
    ap.add_argument("--server-host", default="127.0.0.1")
    ap.add_argument("--loss", type=float, default=0.0)
    ap.add_argument("--delay-ms", type=float, default=0.0)
    ap.add_argument("--jitter-ms", type=float, default=0.0)
    ap.add_argument("--reorder-pct", type=float, default=0.0)
    ap.add_argument("--reorder-ms", type=float, default=0.0)
    ap.add_argument("--log", required=True)
    ap.add_argument("--idle", type=float, default=12.0)
    ap.add_argument("--ready-file", required=True)
    a=ap.parse_args()
    p=Proxy(a.front_port, a.server_host, a.back_port, a.loss, a.delay_ms, a.jitter_ms, a.reorder_pct, a.reorder_ms, a.log, a.idle, a.ready_file)
    p.loop()
