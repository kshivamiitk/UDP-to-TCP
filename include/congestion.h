
#pragma once
#ifndef CONGESTION_H
#define CONGESTION_H
#include <cstdint>
#include <algorithm>
struct CongestionControl {
    using u32 = uint32_t;
    u32 cwnd;
    u32 ssthresh;
    u32 dupAcks;
    bool inFastRec;
    CongestionControl(u32 init_cwnd = 4, u32 init_ssthresh = 32) : cwnd(init_cwnd), ssthresh(init_ssthresh), dupAcks(0), inFastRec(false) {}
    inline void onPacketSent() {}
    inline void onNewAck(u32 newSegsAcked) {
        if (inFastRec) { cwnd = std::max<u32>(ssthresh, 1); inFastRec = false; dupAcks = 0; }
        if (newSegsAcked == 0) return;
        if (cwnd < ssthresh) cwnd += newSegsAcked;
        else { u32 inc = (newSegsAcked >= cwnd) ? 1u : 0u; cwnd += inc; }
    }
    inline bool onDupAck() {
        if (inFastRec) { 
            ++cwnd; 
            return false;
        }
        ++dupAcks;
        if (dupAcks == 3) { 
            ssthresh = std::max<u32>(cwnd / 2, 2); 
            cwnd = ssthresh + 3; 
            inFastRec = true; 
            return true; 
        }
        return false;
    }
    inline void onTimeout() { 
        ssthresh = std::max<u32>(cwnd / 2, 2); 
        cwnd = 1; 
        dupAcks = 0; 
        inFastRec = false; 
    }
    
    inline u32 sendAllowance(u32 inFlight, u32 rwndSegs) const { 
        u32 win = std::min<u32>(cwnd, rwndSegs); 
        return (win > inFlight) ? (win - inFlight) : 0; 
    }
    inline void resetDupAcks() { dupAcks = 0; }
};
#endif
