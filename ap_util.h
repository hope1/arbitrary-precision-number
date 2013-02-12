#ifndef HOPE_BIGNUM_AP_IMPL_H
#define HOPE_BIGNUM_AP_IMPL_H

#define Macro_min(Marg_exp1, Marg_exp2) \
    ((Marg_exp1) < (Marg_exp2) ? (Marg_exp1) : (Marg_exp2))

#define Macro_max(Marg_exp1, Marg_exp2) \
    ((Marg_exp1) < (Marg_exp2) ? (Marg_exp2) : (Marg_exp1))

#define Macro_swap_val(Marg_type, Marg_v1, Marg_v2) \
    do { \
        Marg_type (Mtmp_swap) = (Marg_v1); \
        (Marg_v1) = (Marg_v2); \
        (Marg_v2) = (Mtmp_swap); \
    } while(0)

static inline bool ap_dig_overflow(ap_dig_t r, ap_dig_t a, ap_dig_t b) {
    return r < Macro_min(a, b);
}

// position of most significant bit
static inline size_t ap_dig_msb(ap_dig_t n) {
    size_t r = 0;
    while(n >>= 1)
        ++r;
    return r;
}

struct ap_dig_pair {
    ap_dig_t lo;
    ap_dig_t hi;
};

static inline struct ap_dig_pair ap_dig_mul(ap_dig_t op1, ap_dig_t op2) {
    size_t half = AP_DIG_BIT >> 1;
    // lo and hi parts of operands
    ap_dig_t al = op1 & ((1ULL << half) - 1), ah = op1 >> half,
             bl = op2 & ((1ULL << half) - 1), bh = op2 >> half;
    // let s = 2^(AP_DIG_BIT/2). x * s = x << (AP_DIG_BIT/2)
    // (ah * s + al) * (bh * s + bl)
    // = (ah * bh * s^2) + (al * bh + ah * bl) * s + (al * bl)
    // so hi(res) = ah * bh + hi(t), lo(res) = al * bl + lo(t)
    ap_dig_t t = al * bh + ah * bl;
    bool tcarry = ap_dig_overflow(t, al * bh, ah * bl);

    struct ap_dig_pair res;
    res.lo = (al * bl) + (t << half);
    bool locarry = ap_dig_overflow(res.lo, al * bl, t << half);

    res.hi = (ah * bh) + (t >> half) + (tcarry << half) + locarry;
    return res;
}

#endif // HOPE_BIGNUM_AP_IMPL_H
