#include "congestion.h"
#include <iostream>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    } else {
        std::cout << "[PASS] " << message << '\n';
    }
}
}

int main() {
    using u32 = CongestionControl::u32;

    CongestionControl slowStart(4, 32);
    slowStart.onNewAck(1);
    check(slowStart.cwnd == 5, "Slow start increases cwnd by newly acked segment count");
    slowStart.onNewAck(2);
    check(slowStart.cwnd == 7, "Slow start continues additive growth below ssthresh");

    CongestionControl avoidance(4, 16);
    avoidance.cwnd = 20;
    avoidance.onNewAck(20);
    check(avoidance.cwnd == 21, "Congestion avoidance increases cwnd by one per window of acks");
    avoidance.onNewAck(5);
    check(avoidance.cwnd == 21, "Partial window acknowledgements do not grow cwnd in avoidance");

    CongestionControl fastRecovery(12, 32);
    bool fastRetransmit = false;
    for (int i = 0; i < 3; ++i) {
        fastRetransmit = fastRecovery.onDupAck();
    }
    check(fastRetransmit, "Third duplicate ACK triggers fast retransmit");
    check(fastRecovery.inFastRec, "Fast recovery flag set after triple duplicate ACK");
    check(fastRecovery.ssthresh == 6, "ssthresh reduced to half of cwnd on fast retransmit");
    check(fastRecovery.cwnd == fastRecovery.ssthresh + 3, "cwnd inflated during fast recovery");

    fastRetransmit = fastRecovery.onDupAck();
    check(!fastRetransmit, "Additional duplicate ACKs keep sender in fast recovery");
    check(fastRecovery.cwnd == fastRecovery.ssthresh + 4, "cwnd grows linearly during fast recovery");

    fastRecovery.onNewAck(1);
    check(!fastRecovery.inFastRec, "ACK after retransmit exits fast recovery");
    check(fastRecovery.cwnd == fastRecovery.ssthresh, "cwnd reset to ssthresh when leaving fast recovery");
    check(fastRecovery.dupAcks == 0, "Duplicate ACK counter cleared on recovery");

    CongestionControl timeout(10, 32);
    timeout.onTimeout();
    check(timeout.ssthresh == 5, "Timeout halves cwnd to derive new ssthresh");
    check(timeout.cwnd == 1, "Timeout drops cwnd to one segment");
    check(timeout.dupAcks == 0, "Timeout clears duplicate ACK counter");
    check(!timeout.inFastRec, "Timeout leaves fast recovery state");

    CongestionControl allowance(8, 32);
    allowance.cwnd = 10;
    u32 allowed = allowance.sendAllowance(4, 12);
    check(allowed == 6, "Send allowance limited by congestion window");
    allowed = allowance.sendAllowance(12, 12);
    check(allowed == 0, "Send allowance zero when in-flight equals window");

    if (failures == 0) {
        std::cout << "All congestion control checks passed." << std::endl;
        return 0;
    }

    std::cerr << failures << " congestion control checks failed." << std::endl;
    return 1;
}