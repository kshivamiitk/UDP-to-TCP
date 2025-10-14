import argparse, socket, select, time, random, heapq, json, sys, os
F_SYN=1; F_ACK=2; F_FIN=4; F_DATA=8
def parse_hdr(b):
    if len(b)<13: return None
    flags=b[0]
    seq=int.from_bytes(b[1:5],'big')
    ack=int.from_bytes(b[5:9],'big')
    wnd=int.from_bytes(b[9:11],'big')
    return flags,seq,ack,wnd
def now(): return time.time()
class Proxy:
    def __init__(self, front_port, back_port, server_host, loss, delay_ms, jitter_ms, reorder_pct, reorder_ms, log_path):
        self.fport=front_port; self.bport=back_port
        self.server=(server_host, back_port)
        self.loss=loss; self.delay=delay_ms/1000.0; self.jitter=jitter_ms/1000.0
        self.reorder_pct=reorder_pct/100.0; self.reorder_ms=reorder_ms/1000.0
        self.front=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.front.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.front.bind(("127.0.0.1", self.fport))
        self.back=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.back.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.back.bind(("127.0.0.1", 0))
        self.client=None
        self.q=[]; self.qctr=0
        self.log=open(log_path,"w")
    def sched(self, sock, data, dest, dirlabel):
        if random.random() < self.loss: return
        d=self.delay + (random.random()*2-1.0)*self.jitter
        if random.random() < self.reorder_pct: d += self.reorder_ms
        due = now() + max(0.0, d)
        self.qctr+=1
        heapq.heappush(self.q, (due, self.qctr, sock, data, dest, dirlabel))
    def loop(self):
        idle_deadline = now() + 30.0
        while True:
            t=now()
            while self.q and self.q[0][0] <= t:
                _,_,sock,data,dest,dirlabel = heapq.heappop(self.q)
                try:
                    sock.sendto(data, dest)
                except Exception:
                    pass
                hdr = parse_hdr(data)
                if hdr:
                    flags,seq,ack,wnd = hdr
                    rec = {"t": now(), "dir": dirlabel}
                    if dirlabel=="c2s": rec.update({"seq":seq,"flags":flags})
                    else: rec.update({"ack":ack,"flags":flags})
                    self.log.write(json.dumps(rec)+"\n"); self.log.flush()
                idle_deadline = now() + 5.0
            r,_,_ = select.select([self.front, self.back], [], [], 0.01)
            for s in r:
                try:
                    data, src = s.recvfrom(65535)
                except BlockingIOError:
                    continue
                if s is self.front:
                    self.client = src
                    self.sched(self.back, data, self.server, "c2s")
                else:
                    if self.client:
                        self.sched(self.front, data, self.client, "s2c")
            if not r and not self.q and now() > idle_deadline:
                break
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
    args=ap.parse_args()
    p=Proxy(args.front_port,args.back_port,args.server_host,args.loss,args.delay_ms,args.jitter_ms,args.reorder_pct,args.reorder_ms,args.log)
    p.loop()
