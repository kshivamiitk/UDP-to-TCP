// Compile: g++ -std=c++17 -O2 ls_data_plane_sim.cpp -o ls_sim
// Run: ./ls_sim input_packet.txt
// Defaults: routers.txt, hosts.txt, input_packet.txt in current directory.

#include <bits/stdc++.h>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

static string trim(const string &s){
    size_t a = s.find_first_not_of(" \t\r\n");
    if(a==string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}

uint32_t ipToUint(const string &ip){
    unsigned a,b,c,d;
    char dot;
    stringstream ss(ip);
    if(!(ss >> a >> dot >> b >> dot >> c >> dot >> d)) throw runtime_error("bad ip");
    return (a<<24) | (b<<16) | (c<<8) | d;
}
string uintToIp(uint32_t x){
    return to_string((x>>24)&0xFF) + "." + to_string((x>>16)&0xFF) + "." + to_string((x>>8)&0xFF) + "." + to_string(x&0xFF);
}

struct Host { string filename; string ip; int attachedRouter = -1; };
struct Edge { int v; long long w; };

int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string inputFile = (argc>=2 ? argv[1] : "input_packet.txt");
    if(!fs::exists(inputFile)) {
        cerr << "input file '" << inputFile << "' not found. Create with: <src_host_file> <dst_ip> [start_router]\n";
        return 1;
    }
    unordered_map<string,int> routerIndex;
    vector<string> routerNames;
    vector<tuple<int,int,long long>> edgeList;

    auto ensureRouter = [&](const string &rname)->int{
        auto it = routerIndex.find(rname);
        if(it!=routerIndex.end()) return it->second;
        int idx = (int)routerNames.size();
        routerNames.push_back(rname);
        routerIndex[rname] = idx;
        return idx;
    };

    if(fs::exists("routers.txt")){
        ifstream ifs("routers.txt");
        string line;
        while(getline(ifs,line)){
            line = trim(line);
            if(line.empty() || line[0]=='#') continue;
            stringstream ss(line);
            vector<string> toks;
            string t;
            while(ss >> t) toks.push_back(t);
            if(toks.size()==0) continue;
            if((toks[0]=="Router" || toks[0]=="router") && toks.size()>=2){
                ensureRouter(toks[1]);
            } else if(toks.size()==3){
                string a=toks[0], b=toks[1]; long long w = stoll(toks[2]);
                int ia=ensureRouter(a), ib=ensureRouter(b);
                edgeList.emplace_back(ia,ib,w);
                edgeList.emplace_back(ib,ia,w); // undirected
            } else if(toks.size()==4 && (toks[0]=="Link" || toks[0]=="link")){
                string a=toks[1], b=toks[2]; long long w=stoll(toks[3]);
                int ia=ensureRouter(a), ib=ensureRouter(b);
                edgeList.emplace_back(ia,ib,w);
                edgeList.emplace_back(ib,ia,w);
            } else {
            }
        }
    } else {
        cout << "routers.txt not found -> building default routers R0,R1,R2\n";
        ensureRouter("R0"); ensureRouter("R1"); ensureRouter("R2");
        edgeList.emplace_back(0,1,1); edgeList.emplace_back(1,0,1);
        edgeList.emplace_back(1,2,1); edgeList.emplace_back(2,1,1);
    }

    int R = (int)routerNames.size();
    if(R==0){ cerr << "no routers defined\n"; return 1; }

    vector<vector<Edge>> adj(R);
    for(auto &e: edgeList){
        int u,v; long long w; tie(u,v,w)=e;
        adj[u].push_back({v,w});
    }

    vector<Host> hosts;
    unordered_map<string,int> hostFileToIdx;
    unordered_map<string,int> hostIpToIdx;
    if(fs::exists("hosts.txt")){
        ifstream hfs("hosts.txt");
        string line;
        while(getline(hfs,line)){
            line = trim(line);
            if(line.empty() || line[0]=='#') continue;
            stringstream ss(line);
            string fname, ip, rname;
            if(!(ss >> fname >> ip >> rname)){
                cerr << "malformed hosts.txt line ignored: " << line << "\n";
                continue;
            }
            int ridx;
            if(routerIndex.count(rname)) ridx = routerIndex[rname];
            else ridx = ensureRouter(rname);
            Host h; h.filename = fname; h.ip = ip; h.attachedRouter = ridx;
            int idx = (int)hosts.size();
            hosts.push_back(h);
            hostFileToIdx[fname] = idx;
            hostIpToIdx[ip] = idx;
            if(!fs::exists(fname)){
                ofstream ofs(fname, ios::app);
                ofs << "# host file for " << ip << " attached to " << rname << "\n";
            }
        }
    } else {
        cout << "hosts.txt not found -> creating a couple default hosts\n";
        string fn0="host0.txt", fn1="host1.txt";
        string ip0="10.0.0.1", ip1="10.0.0.2";
        Host h0{fn0, ip0, 0}; hosts.push_back(h0); hostFileToIdx[fn0]=0; hostIpToIdx[ip0]=0; if(!fs::exists(fn0)){ofstream ofs(fn0, ios::app); ofs<<"# default\n";}
        int idx1 = (int)hosts.size();
        Host h1{fn1, ip1, R-1}; hosts.push_back(h1); hostFileToIdx[fn1]=idx1; hostIpToIdx[ip1]=idx1; if(!fs::exists(fn1)){ofstream ofs(fn1, ios::app); ofs<<"# default\n";}
    }

    R = (int)routerNames.size();
    adj.resize(R);

    string srcHostFile, dstIp, startRouterToken;
    {
        ifstream ifs(inputFile);
        string line;
        bool got=false;
        while(getline(ifs,line)){
            line = trim(line);
            if(line.empty()|| line[0]=='#') continue;
            stringstream ss(line);
            ss >> srcHostFile >> dstIp >> startRouterToken;
            got = true;
            break;
        }
        if(!got){ cerr << "input file malformed\n"; return 1; }
    }

    if(!hostFileToIdx.count(srcHostFile)){
        cerr << "source host file '"<<srcHostFile<<"' not found in hosts.txt\n"; return 1;
    }
    if(!hostIpToIdx.count(dstIp)){
        cerr << "destination IP '"<<dstIp<<"' not found in hosts.txt. Simulation requires host defined. Aborting.\n";
        return 1;
    }

    int srcHostIdx = hostFileToIdx[srcHostFile];
    int dstHostIdx = hostIpToIdx[dstIp];
    int srcRouterIdx = hosts[srcHostIdx].attachedRouter;
    int dstRouterIdx = hosts[dstHostIdx].attachedRouter;

    if(!startRouterToken.empty()){
        if(routerIndex.count(startRouterToken)) srcRouterIdx = routerIndex[startRouterToken];
        else {
            bool numeric=true; for(char c:startRouterToken) if(!isdigit(c)) numeric=false;
            if(numeric){
                int v = stoi(startRouterToken);
                if(v>=0 && v<R) srcRouterIdx=v;
            }
        }
    }

    cout << "Simulating: payload from file '"<<srcHostFile<<"' (host IP="<<hosts[srcHostIdx].ip<<")\n";
    cout << "Destination IP: " << dstIp << " attached to router " << routerNames[dstRouterIdx] << "\n";
    cout << "Packet enters network at router: " << routerNames[srcRouterIdx] << "\n\n";
    vector<unordered_map<int,int>> FIB(R);
    for(int r=0;r<R;++r){
        // Dijkstra from r
        const long long INF = (1LL<<60);
        vector<long long> dist(R, INF);
        vector<int> parent(R, -1);
        dist[r]=0;
        using P = pair<long long,int>;
        priority_queue<P, vector<P>, greater<P>> pq;
        pq.push({0,r});
        while(!pq.empty()){
            auto [d,u]=pq.top(); pq.pop();
            if(d!=dist[u]) continue;
            for(auto &ed: adj[u]){
                int v = ed.v; long long w=ed.w;
                if(dist[v] > d + w){
                    dist[v] = d + w;
                    parent[v] = u;
                    pq.push({dist[v], v});
                }
            }
        }
        for(size_t hidx=0; hidx<hosts.size(); ++hidx){
            int hretn = hosts[hidx].attachedRouter;
            if(hretn==r){
                FIB[r][(int)hidx] = -1;
                continue;
            }
            if(dist[hretn] >= INF) {
                FIB[r][(int)hidx] = -2;
                continue;
            }
            int cur = hretn;
            int prev = parent[cur];
            while(prev != -1 && prev != r){
                cur = prev;
                prev = parent[cur];
            }
            if(prev == -1){
                FIB[r][(int)hidx] = -2;
            } else {
                FIB[r][(int)hidx] = cur;
            }
        }
    }

    if(!fs::exists(srcHostFile)){ cerr << "source file missing on disk\n"; return 1; }
    string payload;
    { ifstream sf(srcHostFile, ios::binary); ostringstream ss; ss<<sf.rdbuf(); payload = ss.str(); }

    struct Packet { string dstIp; int dstHostIdx; int ttl; };
    Packet pkt{dstIp, dstHostIdx, 64};

    vector<string> hopLog;
    int curRouter = srcRouterIdx;
    const int MAX_HOPS = 256;
    bool delivered=false;
    for(int hop=0; hop<MAX_HOPS; ++hop){
        hopLog.push_back("At router: " + routerNames[curRouter] + " (hop " + to_string(hop+1) + ")");
        auto it = FIB[curRouter].find(pkt.dstHostIdx);
        if(it==FIB[curRouter].end()){
            hopLog.push_back("  No FIB entry at router -> drop");
            break;
        }
        int next = it->second;
        if(next == -2){
            hopLog.push_back("  Destination unreachable from here -> drop");
            break;
        }
        if(next == -1){
            hopLog.push_back("  Destination host is attached here. Delivering payload to " + hosts[pkt.dstHostIdx].filename);
            ofstream dst(hosts[pkt.dstHostIdx].filename, ios::app | ios::binary);
            time_t t = time(nullptr);
            dst << "\n# Delivered packet from " << srcHostFile << " via routers:";
            for(auto &s: hopLog) dst << "\n#   " << s;
            dst << "\n# Delivered at: " << ctime(&t);
            dst << payload << "\n# EndPacket\n";
            dst.close();
            delivered=true;
            break;
        } else {
            hopLog.push_back("  Forwarding to next router: " + routerNames[next]);
            pkt.ttl--;
            if(pkt.ttl <= 0){
                hopLog.push_back("  TTL expired -> drop");
                break;
            }
            curRouter = next;
            continue;
        }
    }

    cout << "==== Simulation result ====\n";
    for(auto &s: hopLog) cout << s << "\n";
    if(delivered) cout << "\nPacket delivered successfully to " << hosts[pkt.dstHostIdx].filename << "\n";
    else cout << "\nPacket NOT delivered (dropped or unreachable)\n";

    cout << "\nFIBs (installed forwarding entries):\n";
    for(int r=0;r<R;++r){
        cout << "Router " << routerNames[r] << ":\n";
        for(size_t h=0; h<hosts.size(); ++h){
            int nh = FIB[r][(int)h];
            cout << "  dst ip=" << hosts[h].ip << " -> ";
            if(nh==-1) cout << "LOCAL\n";
            else if(nh==-2) cout << "UNREACHABLE\n";
            else cout << "next-hop=" << routerNames[nh] << "\n";
        }
    }

    return 0;
}
