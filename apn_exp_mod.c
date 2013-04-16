#include "apn.h"
#include "apz.h"
#include "ap_impl.h"

// TODO
void apn_sqr(apn_s* res, const apn_s* op) {
    apn_mul(res, op, op);
}

void apn_exp(apn_s* res, const apn_s* base, const apn_s* exp) {
    if(exp->_size == 1) {
        switch(exp->_data[0])
        {
        case 0:
            apn_assign_dig(res, 1);
            return;
        case 1:
            apn_assign(res, base);
            return;
        case 2:
            apn_sqr(res, base);
            return;
        }
    }
    apn_exp_bysqr(res, base, exp);
}

void apn_exp_dig(apn_s* res, const apn_s* base, ap_dig_t exp) {
    if(!exp) {
        apn_assign_dig(res, 1);
        return;
    }
    apn_s B;
    apn_init(&B);
    apn_assign(&B, base);

    apn_assign(res, &B);
    // by squaring..
    size_t i = ap_dig_msb(exp);
    for(; i--;) {
        apn_sqr(res, res);
        if(exp & (1 << i))
            apn_mul(res, res, &B);
    }

    apn_clear(&B);
}

// Exponentiation by squaring
// exp != 0
void apn_exp_bysqr(apn_s* res, const apn_s* base, const apn_s* exp) {
    apn_s Z;
    apn_init(&Z);
    // Left-to-right in exp, 0 = square, 1 = square and multiply by base
    apn_assign(&Z, base); // ignore first 1 in exp
    for(size_t i = exp->_size, j = ap_dig_msb(exp->_data[exp->_size - 1]); i--;) {
        for(; j--;) {
            apn_sqr(&Z, &Z);
            if(exp->_data[i] & (1 << j))
                apn_mul(&Z, &Z, base);
        }
        j = AP_DIG_BIT;
    }
    apn_swap(res, &Z);
    apn_clear(&Z);
}

void apn_modexp(apn_s* res, const apn_s* base, const apn_s* exp, const apn_s* mod) {
    // Modular exponentiation using repeated squaring.
    // a[i] represents bit i of a.
    // 1. e   := sum     [i = 0, bitof e] 2^i * e[i],
    //    b^e := product [i = 0, bitof e] b^(2^i * e[i]).
    // 2. a * b (mod m) = a * (b (mod m)) (mod m).
    apn_s r, e, b;
    apn_init_list(&r, &e, &b, NULL);
    apn_assign_dig(&r, 1);
    apn_assign(&e, exp);
    apn_assign(&b, base);
    // b = base^(2^i) % m
    while(!apn_is_zero(&e)) {
        if(apn_is_odd(&e)) { // current term is needed only if e[i] != 0
            apn_mul(&r, &r, &b);
            apn_div(NULL, &r, &r, mod);
        }
        apn_bit_shr(&e, &e, 1);
        // maintain b
        apn_sqr(&b, &b);
        apn_div(NULL, &b, &b, mod);
    }
    apn_swap(&r, res);
    apn_clear_list(&r, &e, &b, NULL);
}
