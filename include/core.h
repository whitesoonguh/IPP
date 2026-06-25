#ifndef _CORE_H
#define _CORE_H

#include <vector>
#include <stdexcept>
#include <mcl/bls12_381.hpp>

using namespace std;
using namespace mcl::bn;

typedef vector<Fr> FrVec;

// Helper for Sanity Check
static inline bool is_pow2(uint32_t n) { return n != 0 && (n & (n - 1)) == 0; }


// Inplace Batch Inversion from Montgomery Trick
void batch_invert(FrVec &x);

// Transcript Struct for FS Transform 
struct Transcript {
    vector<uint8_t> buf;
    Fr state;
    Transcript(const string& label) {
        buf.reserve(2048);
        state.setHashOf(label.data(), label.size());
    }

    void absorb(const GT &x) {
        uint8_t tmp[1024];
        size_t w = x.serialize(tmp, sizeof(tmp));
        buf.insert(buf.end(), tmp, tmp + w);
    }

    Fr challenge() {
        uint8_t sb[64];
        size_t sw = state.serialize(sb, sizeof(sb));
        buf.insert(buf.end(), sb, sb + sw);
        Fr c;
        c.setHashOf(buf.data(), buf.size());
        state = c;
        buf.clear();
        return c;
    }
};

// Inner Pairing Proof Prover
struct IPPproof {
    GT P;
    vector<GT> tx;
    G1 w;
    G2 g;
    G1* ww;
    G2* gg;

    void init(G2 *gg_in, G1* ww_in);
    void prove(vector<G2> gg, GT P, vector<G1> ww);
};

// Inner Pairing Proof Verifier
bool IPPverify(IPPproof *pi);

#endif 