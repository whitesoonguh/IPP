#include "core.h"

// Inplace Batch Inversion from Montgomery Trick
void batch_invert(FrVec &x) {
    uint32_t n = x.size();
    FrVec acc(n);
    acc[0] = x[0];
    for (uint32_t i = 1; i < n; i++) {
        acc[i] = acc[i-1] * x[i];
    }
    Fr accInv = 1 / acc[n-1];
    for (uint32_t i = n - 1; i > 0; i--) { 
        Fr xi = x[i];                    
        x[i] = acc[i-1] * accInv;        
        accInv *= xi;                    
    }
    x[0] = accInv;                      
}



// IPP Proof 
void IPPproof::init(G2 *gg_in, G1* ww_in) {
    this->gg = gg_in;
    this->ww = ww_in;
    this->tx.clear();
}

// Note: gg, ww: mutable, so we just copy them.
void IPPproof::prove(vector<G2> gg, GT P, vector<G1> ww){
    uint32_t n = ww.size();
    GT l, r; Fr c, cinv; string buf;
    Transcript tr("IPP-C&A LAB");
    tr.absorb(P);

    while (n > 1) {
        n>>=1;
        millerLoopVec(l, &ww[n], &gg[0], n); millerLoopVec(r, &ww[0], &gg[n], n);
        finalExp(l, l); finalExp(r, r);
        tx.push_back(l); tx.push_back(r);
        tr.absorb(l); tr.absorb(r);
        c = tr.challenge();
        cinv = 1/c;

        for (uint32_t i = 0; i < n; i++) {
            ww[i]=ww[i]+(ww[i + n]*(c));
            gg[i]=gg[i]+(gg[i + n]*(cinv));                
        }
    }
    this->P = P;
    this->w = ww[0];
    this->g = gg[0];
}

bool IPPverify(IPPproof *pi) {
    
    uint32_t lgn = pi->tx.size() / 2;

    // Compute Left
    vector<GT> base = move(pi->tx);
    Fr c; GT P_v;
    FrVec c_vec(lgn);
    FrVec expon(2*lgn);

    Transcript tr("IPP-C&A LAB");
    tr.absorb(pi->P);

    for(uint32_t i=0; i<lgn; i++){
        tr.absorb(base[2*i]); tr.absorb(base[2*i + 1]);
        c = tr.challenge();
        c_vec[i] = c;
        expon[2*i] = c;
    }
    batch_invert(c_vec);
    for (uint32_t i = 0; i < lgn; i++) {
        expon[2*i+1] = c_vec[i];
    }

    GT::powVec(P_v, &base[0], &expon[0], 2*lgn);
    P_v = P_v * pi->P;
        
    // Compute Right    
    uint32_t n = 1<<lgn;
    expon.clear(); expon.resize(n);
    expon[0] = Fr(1);
       
    // Prepare Indices 
    for (uint32_t i = 1; i < n; i++) {
        uint32_t lgi = 31 - __builtin_clz(i);
        uint32_t k = 1<<lgi;
        Fr u = c_vec[lgn - lgi - 1];                
        expon[i] = expon[i-k] * u;        
    }
    
    // Do MSM & Pairing
    G2 g_v;
    G2::mulVec(g_v, pi->gg, &expon[0], n);
    GT res;
    pairing(res, pi->w, g_v);

    // Check Left == Right
    bool flag=(P_v==res);
    return flag;
}