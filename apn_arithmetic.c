#include "apn.h"
#include "ap_util.h"

size_t apn_data_add(ap_dig_t* rp, const ap_dig_t* ap, size_t an,
                    const ap_dig_t* bp, size_t bn) {
    size_t max_size = Macro_max(an, bn) + 1;

    bool carry = false;
    size_t i = 0;
    for(; i != Macro_min(an, bn); ++i) {
        ap_dig_t t = ap[i] + bp[i];
        bool tcarry = ap_dig_overflow(t, ap[i], bp[i]);

        rp[i] = t + carry;
        carry = tcarry || ap_dig_overflow(rp[i], t, carry); // carry will not overflow if t did
    }
    const ap_dig_t* p = (i < an) ? ap : bp;
    for(; i != max_size - 1; ++i) {
        ap_dig_t t = p[i] + carry;
        carry = ap_dig_overflow(t, p[i], carry);
        rp[i] = t;
    }
    if(carry)
        rp[max_size - 1] = 1;

    return max_size - !carry;
}

void apn_add(apn_s* res, const apn_s* op1, const apn_s* op2) {
    size_t max_size = Macro_max(op1->_size, op2->_size) + 1;
    if(res->_capacity < max_size)
        apn_realloc(res, max_size);

    res->_size = apn_data_add(res->_data, op1->_data, op1->_size,
                              op2->_data, op2->_size);
}

void apn_add_dig(apn_s* res, const apn_s* op, ap_dig_t dig) {
    apn_assign(res, op);
    ap_dig_t *p = res->_data;

    ap_dig_t t = *p;
    *p += dig;
    if(ap_dig_overflow(*p, t, dig)) {
        ++p;
        while(p != res->_data + res->_size && *p == AP_DIG_MAX)
            *p++ = 0;

        if(p == res->_data + res->_size) {
            if(res->_capacity == res->_size)
                apn_realloc(res, res->_size + 1);
            res->_data[res->_size++] = 1;
        } else
            *p += 1;
    }
}

size_t apn_data_sub(ap_dig_t* rp, const ap_dig_t* ap, size_t an,
                    const ap_dig_t* bp, size_t bn) {
    size_t max_size = Macro_max(an, bn);

    bool borrow = false;
    size_t i = 0;
    for(; i != Macro_min(an, bn); ++i) {
        ap_dig_t t = ap[i] - bp[i];
        bool tborrow = t > ap[i]; // underflow

        rp[i] = t - borrow;
        borrow = tborrow || rp[i] > t;
    }
    const ap_dig_t* p = (i < an) ? ap : bp;
    for(; i != max_size; ++i) {
        ap_dig_t t = p[i] - borrow;
        borrow = t > p[i];

        rp[i] = t;
    }
    while(--i && !rp[i]);

    return borrow ? 0 : i + 1;
}

void apn_sub(apn_s* res, const apn_s* op1, const apn_s* op2) {
    size_t max_size = Macro_max(op1->_size, op2->_size);
    if(res->_capacity < max_size)
        apn_realloc(res, max_size);

    res->_size = apn_data_sub(res->_data, op1->_data, op1->_size, op2->_data, op2->_size);
    if(!res->_size) // underflow
        apn_assign_dig(res, 0);
}

void apn_sub_dig(apn_s* res, const apn_s* op, ap_dig_t dig) {
    apn_assign(res, op);
    ap_dig_t *p = res->_data;

    ap_dig_t t = *p;
    *p -= dig;
    if(*p > t) {
        ++p;
        while(p != res->_data + res->_size && *p == 0)
            *p++ = AP_DIG_MAX;
        // if(p == res->_data + res->_size) underflow;
        *p -= 1;
    }
}

void apn_mul(apn_s* res, const apn_s* op1, const apn_s* op2) {
    size_t n = Macro_max(op1->_size, op2->_size);
    if(n < APN_MUL_KARATSUBA_THRESHOLD)
        apn_mul_basecase(res, op1, op2);
    else
        apn_mul_karatsuba(res, op1, op2);
}

// long multiplication
void apn_mul_basecase(apn_s* res, const apn_s* op1, const apn_s* op2) {
    size_t max_size = op1->_size + op2->_size;

    apn_s r;
    apn_init(&r);
    apn_realloc(&r, max_size);
    apn_data_fill_zero(&r);

    for(size_t i = 0; i != op1->_size; ++i) {
        for(size_t j = 0; j != op2->_size; ++j) {
            struct ap_dig_pair x = ap_dig_mul(op1->_data[i], op2->_data[j]);
            ap_dig_t* p = r._data + i + j;
            apn_data_add(p, p, max_size - i - j, &x.lo, 2);
        }
    }
    size_t i = max_size - 1;
    while(i && !r._data[i]) --i;
    r._size = i + 1;

    apn_swap(res, &r);
    apn_clear(&r);
}

void apn_mul_karatsuba(apn_s* res, const apn_s* op1, const apn_s* op2) {
    // Split operands at m: x = x1 B^m + x0, y = y1 B^m + y0.
    // xy = z2 B^2m + z1 B^m + z0, z0 = x0y0, z1 = x0y1 + x1y0, z2 = x1y1
    // This requires 4 multiplications, but z1 can be calculated as
    // z1 = (x1 + x0)(y1 + y0) - z2 - z0
    size_t m = Macro_max(op1->_size, op2->_size) / 2; // dividing point

    if(m <= APN_MUL_KARATSUBA_THRESHOLD) { // base case
        apn_mul_basecase(res, op1, op2);
        return;
    }

    apn_s x0, x1, y0, y1, z0, z1, z2, t; // result and parts, temp
    apn_init_list(&x0, &x1, &y0, &y1, &z0, &z1, &z2, &t, NULL);

    apn_assign_part(&x0, op1, 0, Macro_min(op1->_size, m));
    if(op1->_size > m)
        apn_assign_part(&x1, op1, m, op1->_size - m);

    apn_assign_part(&y0, op2, 0, Macro_min(op2->_size, m));
    if(op2->_size > m)
        apn_assign_part(&y1, op2, m, op2->_size - m);

    apn_mul_karatsuba(&z0, &x0, &y0);
    apn_mul_karatsuba(&z2, &x1, &y1);
    // z1
    apn_add(&t, &x0, &x1);
    apn_add(&z1, &y0, &y1);
    apn_mul_karatsuba(&z1, &z1, &t);
    apn_add(&t, &z0, &z2);

    apn_sub(&z1, &z1, &t);
    // shift and add to result
    apn_shl(&z2, &z2, 2 * m);
    apn_add(res, &z0, &z2);
    apn_shl(&z1, &z1, m);
    apn_add(res, res, &z1);

    apn_clear_list(&x0, &x1, &y0, &y1, &z0, &z1, &z2, &t, NULL);
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

// [op1 / op2], where the quotient is a single digit
static inline ap_dig_t apn_div_aux_fast(apn_s* rem, const apn_s* op1, const apn_s* op2) {
    // Let A denote the one or two last digit of op1 depending on size of op2,
    // B denote op2[-1] + 1, first guess a approximate quotient by taking [A / B],
    // this result must be <= the real quotient. The difference between real
    // and guessed quotient is op1 / op2 - [A / B] = (op1 - [A / B] * op2) / op2,
    // which is another such division that can we can apply the same method again.
    // To obtain the result, the quotient is the sum of [A / B] in each iteration,
    // and remainder is the difference op1 - [A / B] * op2 at the last iteration.
    apn_s t, r;
    apn_init_list(&t, &r, NULL);
    apn_assign(&r, op1);
    ap_dig_t q = 0;
    while(true) { // break later
        bool f = (r._size > op2->_size);
        struct ap_dig_pair a = { .lo = r._data[r._size - f - 1],
                                 .hi = f ? r._data[r._size - 1] : 0 };
        ap_dig_t b = op2->_data[op2->_size - 1] + 1;

        if(r._size < op2->_size || (!a.hi && a.lo < b))
            break;

        ap_dig_t v = ap_dig_div_2d1t1(a, b);
        q += v;
        apn_assign_dig(&t, v);
        apn_mul(&t, &t, op2);
        apn_sub(&r, &r, &t);
    }
    if(apn_cmp(&r, op2) >= 0) {
        ++q;
        apn_sub(&r, &r, op2);
    }

    apn_swap(&r, rem);
    apn_clear_list(&r, &t, NULL);
    return q;
}

// long division
void apn_div(apn_s* dest_quot, apn_s* dest_rem, const apn_s* op1, const apn_s* op2) {
    apn_s quot, rem;
    apn_init_list(&quot, &rem, NULL);
    // msb to lsb
    size_t i = op1->_size - 1;
    do {
        apn_shl(&rem, &rem, 1);
        rem._data[0] = op1->_data[i];
        ap_dig_t tq = 0; // current digit of quotient
        
        if(apn_cmp(&rem, op2) >= 0)
            tq = apn_div_aux_fast(&rem, &rem, op2);
        // add to current quotient
        apn_shl(&quot, &quot, 1);
        quot._data[0] = tq;
    } while(i--);

    if(dest_quot != NULL)
        apn_swap(&quot, dest_quot);
    if(dest_rem != NULL)
        apn_swap(&rem, dest_rem);
    apn_clear_list(&quot, &rem, NULL);
}
