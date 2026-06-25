# Inner Pairing Product (IPP)

A compact, from-scratch implementation of a **logarithmic-size inner pairing product argument** over the BLS12-381 curve, built on [`herumi/mcl`](https://github.com/herumi/mcl).

Given two committed vectors — witnesses $\mathbf{w} = (w_0, \dots, w_{n-1}) \in \mathbb{G}_1^n$ and public generators $\mathbf{g} = (g_0, \dots, g_{n-1}) \in \mathbb{G}_2^n$ — the prover convinces a verifier that it knows $\mathbf{w}$ satisfying

$$
P \;=\; \prod_{i=0}^{n-1} e(w_i,\, g_i) \;\in\; \mathbb{G}_T
$$

while sending only $O(\log n)$ target-group elements and the two final folded points. Verification costs **one $\mathbb{G}_2$ multi-scalar multiplication and a single pairing**, instead of the $n$ pairings the naive check would need.

---

## How it works

The argument is an IPA-style recursive fold (à la Bulletproofs) lifted into the pairing setting. Each round halves the problem.

**Prover** — for a current half-width $n' = n/2$:

1. Commit the two cross terms
   $$L = \prod_{i<n'} e(w_{n'+i},\, g_i), \qquad R = \prod_{i<n'} e(w_i,\, g_{n'+i})$$
   (computed as a batched `millerLoopVec` + a single `finalExp`).
2. Derive a Fiat–Shamir challenge $c$ from the transcript.
3. Fold both vectors in place:
   $$w_i \leftarrow w_i + c\cdot w_{n'+i}, \qquad g_i \leftarrow g_i + c^{-1}\cdot g_{n'+i}.$$

After $\log_2 n$ rounds the vectors collapse to a single $w \in \mathbb{G}_1$ and $g \in \mathbb{G}_2$. The proof is $\{(L_j, R_j)\}_{j}$ together with $(w, g)$.

**Verifier**:

1. Replay the transcript to recover every challenge $c_j$.
2. **Left** — rebuild the folded statement directly from the cross terms:
   $$P_v = P \cdot \prod_j L_j^{\,c_j}\, R_j^{\,c_j^{-1}} \quad(\text{one } \mathbb{G}_T\text{ multi-exponentiation}).$$
3. **Right** — rebuild the folded generator from the *original* CRS. The folding coefficients form the standard $s$-vector, where each $s_i$ is a product of the $c_j^{-1}$ selected by the bits of $i$. It is built in $O(n)$ by dynamic programming:
   ```cpp
   uint32_t lgi = 31 - __builtin_clz(i);   // floor(log2 i), MSB position
   uint32_t k   = 1u << lgi;
   expon[i] = expon[i - k] * c_vec[lgn - lgi - 1];
   ```
   then $g_v = \sum_i s_i\, g_i$ via `G2::mulVec`.
4. **Accept iff** $P_v = e(w,\, g_v)$.

### Fiat–Shamir transcript

Challenges are non-interactive via a chained transcript (`include/core.h`):

- **Binary absorption.** Group elements are fed in through mcl's `serialize` (raw bytes), not `getStr` — no decimal/hex formatting, no `std::string` churn on the hot path.
- **Chaining.** Each challenge is $c \leftarrow H(\text{state} \,\|\, \text{absorbed})$ and the running `state` is updated to $c$, so every challenge binds the entire prior transcript rather than just its own round.
- **Domain separation.** The transcript is seeded with a label (`"IPP-C&A LAB"`); prover and verifier share the exact same absorb order, which keeps the two sides byte-identical by construction.

### Batch inversion

The per-round $c^{-1}$ on the verifier side are produced together with Montgomery's trick (`batch_invert`): $n$ inverses for the price of **one** field inversion plus $3n$ multiplications.

---

## Build

The only dependency is mcl, vendored at `./mcl`.

### Native

```bash
git clone https://github.com/whitesoonguh/IPP
cd IPP
git clone --depth 1 https://github.com/herumi/mcl   # provides ./mcl

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

./build/test
```

Requires a C++17 compiler and CMake ≥ 3.16. The build pins `-O2 -march=native`, so compile on the machine you intend to run on (or swap to `-march=x86-64-v3` for portable binaries).

### Docker

A self-contained build that clones mcl at image-build time:

```bash
docker build -t ipp .
docker run --rm ipp /app/build/test
```

---

## Running

`./build/test` runs a self-checking benchmark sweep over $N = 128 \dots 16384$ (powers of two). For each size it builds a random instance, proves, verifies, and reports timing:

```
==================== N = 128, PASSED? 1 ====================
Proving Time (us): …        Amortized: …
Verification Time (us): …   Amortized: …
====================  N = 256, PASSED? 1 ====================
…
```

`PASSED? 1` means the proof verified. The `Amortized` columns divide wall-clock time by $N$, which is the meaningful figure as $N$ grows — verification is dominated by one size-$N$ MSM plus a single pairing, so per-element cost should fall off sharply compared to the naive $N$-pairing baseline.

---

## Layout

```
IPP/
├── include/core.h     # IPPproof + Transcript declarations, batch_invert
├── src/core.cpp       # prover, verifier, Montgomery batch inversion
├── src/main.cpp       # benchmark / self-test harness
├── CMakeLists.txt     # vendored-mcl build (static link, -O2 -march=native)
└── Dockerfile         # ubuntu:24.04, clones herumi/mcl at build time
```

---

## Notes & limitations

This is a research-grade prototype for benchmarking and experimentation, **not** a hardened library.

- **Power-of-two sizes only.** The folding loop assumes $n$ is a power of two; other sizes break the halving. Guard the input if you wire this into anything else.
- **CRS handled as raw pointers.** `IPPproof::init` stores bare `G1*/G2*` without a length, and the verifier trusts the caller passed at least $N$ valid elements into `mulVec`. There is no bounds checking — keep the CRS and its size together at the call site.
- **Not constant-time / not audited.** No side-channel hardening; do not use as-is in production.
- **Statement binding.** The transcript absorbs $P$ and the per-round $L, R$. If you embed this in a larger protocol, bind the full public statement into the transcript.

---

## Background

The technique is the recursive inner-product folding of Bootle et al. (EUROCRYPT 2016) and Bulletproofs (Bünz et al., IEEE S&P 2018; ePrint 2017/1066), specialized to pairing products as in *Proofs for Inner Pairing Products and Applications* (Bünz, Maller, Mishra, Tyagi, Vesely; ePrint 2019/1177). This repository is an independent, minimal reimplementation of the core argument rather than any specific paper's full system.

---

## License

No license file is currently provided. Add one (e.g. MIT or Apache-2.0) before reuse or distribution.