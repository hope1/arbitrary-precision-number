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
    enum { half = AP_DIG_BIT >> 1 };
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

    res.hi = (ah * bh) + (t >> half) + ((ap_dig_t)tcarry << half) + locarry;
    return res;
}

// 2 / 1 -> 1 division, only quotient is needed
static inline ap_dig_t ap_dig_div_2d1t1(struct ap_dig_pair a, ap_dig_t b) {
    // let B = 1 << AP_DIG_BIT, compute (a.hi * B + a.lo) / b,
    // divide each number x as q = x / b, r = x % b, x = q * b + r.
    // substitute back to the original formula expands to
    // qH*B*b + qH*rB + qB*rH + qL + (rH*rB+rL)/b, but qH will alway be 0
    // since a.hi < b, so the final form is qB*a.hi + qL + (rH*rB+rL)/b.
    // the term (rH*rB+rL) might be > B so that another 2d1t1 division is
    // need, this can be done by a tail recursive call which we turn into a
    // loop easily.

    // B = AP_DIG_MAX + 1
    ap_dig_t qB = AP_DIG_MAX / b, rB = AP_DIG_MAX % b;
    if(rB == b - 1)
        rB = 0, ++qB;
    else
        ++rB;

    ap_dig_t res = 0;
    do {
        ap_dig_t qL = a.lo / b, rL = a.lo % b;
        res += a.hi * qB + qL;

        a = ap_dig_mul(a.hi, rB);
        ap_dig_t s = a.lo;

        a.lo += rL;
        if(ap_dig_overflow(a.lo, s, rL))
            ++a.hi;
    } while(a.hi);
    res += a.lo / b;

    return res;
}

#endif // HOPE_BIGNUM_AP_IMPL_H
