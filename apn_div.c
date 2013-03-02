#include "apn.h"
#include "ap_impl.h"
#include <assert.h>

// [op1 / op2], where the quotient is a single digit
static inline ap_dig_t apn_div_aux(apn_s* rem, const apn_s* op1, const apn_s* op2) {
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

void apn_div(apn_s* dest_quot, apn_s* dest_rem, const apn_s* op1, const apn_s* op2) {
    if(apn_is_zero(op2)) { // division by zero
        volatile int x = 0, y = 1;
        (void)(y / x); // so do it
        return;
    }
    if(apn_cmp(op1, op2) < 0) {
        apn_assign(dest_rem, op1);
        apn_assign_dig(dest_quot, 0); 
        return;
    }
    apn_div_bz(dest_quot, dest_rem, op1, op2);
}

// long division
void apn_div_basecase(apn_s* dest_quot, apn_s* dest_rem, const apn_s* op1, const apn_s* op2) {
    apn_s quot, rem;
    apn_init_list(&quot, &rem, NULL);

    // msb to lsb
    size_t i = op1->_size - op2->_size; // skip first op2.size - 1 digits
    if(op1->_size < op2->_size) {
        apn_assign(dest_rem, op1);
        apn_assign_dig(dest_quot, 0); 
        return;
    }
    apn_assign_part(&rem, op1, i + 1, op1->_size - i - 1);
    do {
        apn_shl(&rem, &rem, 1);
        rem._data[0] = op1->_data[i];
        ap_dig_t tq = 0; // current digit of quotient
        
        if(apn_cmp(&rem, op2) >= 0)
            tq = apn_div_aux(&rem, &rem, op2);
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

// Burnikel-Ziegler divison, see
// Christoph Burnikel and Joachim Ziegler, "Fast Recursive Division", October 1998

void apn_div_bz(apn_s* dest_quot, apn_s* dest_rem, const apn_s* op1, const apn_s* op2) {
//    if(op1->_size <= APN_DIV_BZ_THRESHOLD) {
//        apn_div_basecase(dest_quot, dest_rem, op1, op2);
//        return;
//    }
    // r-digit divide by s-digit number
    apn_s A, B, Z, Q, R, V; // A, B: copy of operands, Q, R: results, Z, V: temp
    apn_init_list(&A, &B, &Z, &Q, &R, &V, NULL);
    apn_assign(&A, op1);
    apn_assign(&B, op2);
    // min{2^k | 2^k * BLOCKSIZE > s}, number of blocks to be divided
    size_t m = (B._size / APN_DIV_BZ_BLOCKSIZE);
    size_t t = !(!m || m & (m - 1)); // is power of 2?
    // m = next power of 2
    while(m >>= 1)
        ++t;
    m = 1 << t;
    // n = ceil(s/m) * m
    size_t n = (B._size / m + (bool)(B._size % m)) * m;
    // extend and normalize B, shift the same amount for A
    apn_shl(&B, &B, n - B._size);
    apn_shl(&A, &A, n - B._size);
    size_t sigma = AP_DIG_BIT - ap_dig_msb(B._data[B._size - 1]) - 1;
    apn_bit_shl(&B, &B, sigma);
    apn_bit_shl(&A, &A, sigma);
    // find t = min {l >= 2 | A < base^(l*n) / 2}
    apn_assign_dig(&V, 1);
    apn_bit_shl(&V, &V, AP_DIG_BIT - 1); // base / 2
    apn_shl(&V, &V, 2 * n - 1);
    t = 2;
    while(apn_cmp(&A, &V) >= 0) {
        apn_shl(&V, &V, n);
        ++t;
    }
    // main loop
    // Z[t-2] = (A[t-1], A[t-2])
    apn_assign_part_zero(&Z, &A, (t - 2) * n, 2 * n);
    size_t i = t - 2;
    do {
        apn_div_basecase(&V, &R, &Z, &B); // TESTING
//        apn_div_bz_d2n1n(&V, &R, &Z, &B);
        apn_shl(&Q, &Q, n);
        apn_add(&Q, &Q, &V);
        if(i != 0) { // set Z = (R, A[i - 1])
            apn_assign(&Z, &R);
            apn_shl(&Z, &Z, n);
            apn_assign_part_zero(&V, &A, (i - 1) * n, n);
            apn_add(&Z, &Z, &V);
        }
    } while(i-- != 0);
    // reduce remainder
    apn_shr(&R, &R, n - B._size);
    apn_bit_shr(&R, &R, sigma);

    apn_swap(dest_quot, &Q);
    apn_swap(dest_rem, &R);
    apn_clear_list(&A, &B, &Z, &Q, &R, &V, NULL);
}

