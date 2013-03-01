#include "apn.h"
#include "apz.h"
#include "ap_impl.h"

// TODO
void apn_sqr(apn_s* res, const apn_s* op) {
    apn_mul(res, op, op);
}

// Exponentiation by squaring
void apn_exp_bysqr(apn_s* res, const apn_s* base, const apn_s* exp) {
    // Left-to-right in exp, 0 = square, 1 = square and multiply by base
    apn_s b; // store base
    apn_init(&b);
    apn_assign(&b, base);

    apn_assign(res, base); // ignore first 1 in exp
    for(size_t i = exp->_size; i-- != 0;) {
        for(size_t j = (i == exp->_size - 1) ?
                       ap_dig_msb(exp->_data[i]) : AP_DIG_BIT; j-- != 0;) {
            apn_sqr(res, res);
            if(exp->_data[i] & (1 << j))
                apn_mul(res, res, &b);
        }
    }
    apn_clear(&b);
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

void apn_modexp(apn_s* res, const apn_s* base, const apn_s* exp, const apn_s* mod) {
    // Represent exp as e = sum[i = 0, n - 1, Ai * 2^i], so
    // b^e = product[i = 0, n - 1, b^(2^i*Ai)].
    // (a * b) % m = (a * (b % m)) % m
    apn_s r, e, b;
    apn_init_list(&r, &e, &b, NULL);
    apn_assign_dig(&r, 1);
    apn_assign(&e, exp);
    apn_assign(&b, base);
    // for every bit in exp
    // b = base^(2^i) % m
    while(!apn_is_zero(&e)) {
        if(apn_is_odd(&e)) { // current term is needed only if Ai != 0
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

// Euclidean algorithm
void apn_gcd(apn_s* res, const apn_s* op1, const apn_s* op2) {
    // gcd(a, b) = gcd(b, a % b) if b != 0, and = a when b == 0
    apn_s b, t; // `res` will serve as a. t is temp object.
    apn_init_list(&b, &t, NULL);
    apn_assign(&t, op1);
    apn_assign(&b, op2);
    apn_swap(res, &t);

    while(!apn_is_zero(&b)) {
        apn_div(NULL, &t, res, &b);
        apn_assign(res, &b);
        apn_assign(&b, &t);
    }

    apn_clear_list(&b, &t, NULL);
}

// `a` and `mod` are coprime
void apn_modinv(apn_s* inv, const apn_s* a, const apn_s* mod) {
    // Extended Euclidean
    // solve x in equation ax + y mod = 1, x modulo `mod` is the
    // inverse of `a` modulo `mod`. Perform normal euclidean algorithm
    // to find the gcd, but with some additional step to calculate x.
    // Initial value x0 = 1, x1 = 0, and at each step x[i] is
    // calculated by x[i] = x[i-2] - q[i] x[i-1] where q[i] is the
    // quotient from the current step. The second-last x[i] when b == 0
    // is the solution.
    apz_s b, r, q, x0, x1, gcd; // `gcd` serves as a, it should be 1 at end
                                // for valid input. x0, x1 is the previous
                                // and current x[i]. r, q stores the result
                                // of euclidean division in each step.
    apz_init_list(&b, &r, &q, &x0, &x1, &gcd, NULL);
    apz_assign_n(&gcd, a);
    apz_assign_n(&b, mod);

    apn_assign_dig(&x0.magnitude, 1);
    apn_assign_dig(&x1.magnitude, 0);

    while(!apn_is_zero(&b.magnitude)) {
        apz_div(&q, &r, &gcd, &b);
        apz_assign(&gcd, &b);
        apz_assign(&b, &r);

        apz_swap(&x0, &x1);
        apz_mul(&r, &q, &x0);
        apz_sub(&x1, &x1, &r);
    }
    apz_mod_n(inv, &x0, mod);

    apz_clear_list(&b, &r, &q, &x0, &x1, &gcd, NULL);
}
