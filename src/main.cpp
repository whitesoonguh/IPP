#include "core.h"
#include <iostream>
#include <chrono>

using namespace std::chrono;

void IPP_test() {
    vector<G2> gg;
    vector<G1> ww;

    for (uint32_t size = 128; size <= 16384; size <<=1) {
        gg.resize(size);
        ww.resize(size);

        Fr r, s;
        r.setByCSPRNG();

        for (uint32_t i = 0; i < size; i++) {
            s = r * Fr(i);
            hashAndMapToG1(ww[i], s.serializeToHexStr());
            hashAndMapToG2(gg[i], s.serializeToHexStr());
        }
        GT P;
        millerLoopVec(P, &ww[0], &gg[0], size);
        finalExp(P, P);

        IPPproof proof; proof.init(&gg[0], &ww[0]);
        // Proof Generation
        auto start_p = steady_clock::now();
        proof.prove(gg, P, ww);
        auto end_p = steady_clock::now();

        // Vefification
        auto start_v = steady_clock::now();
        bool ret = IPPverify(&proof);
        auto end_v = steady_clock::now();

        auto elapsed_p = duration_cast<microseconds>(end_p - start_p).count();
        auto elapsed_v = duration_cast<microseconds>(end_v - start_v).count();

        cout << "==================== " << "N = " << size << ", PASSED? " << ret << " ====================" << endl;
        cout << "Proving Time (us): " << elapsed_p << "\t Amortized: " << elapsed_p / size << endl;
        cout << "Verification Time (us): " << elapsed_v << "\t Amortized: " << elapsed_v / size << endl;
    }
}


int main() {
    initPairing(mcl::BLS12_381);
    IPP_test();
    return 0;
}