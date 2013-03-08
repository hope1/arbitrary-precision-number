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
        apn_sub(&r, &r, op2);
        ++q;
    }

    apn_swap(&r, rem);
    apn_clear_list(&r, &t, NULL);
    return q;
}

void apn_div(apn_s* quot, apn_s* rem, const apn_s* op1, const apn_s* op2) {
    if(apn_is_zero(op2)) { // division by zero
        volatile int x = 0, y = 1;
        (void)(y / x); // so do it
        return;
    }
    if(apn_cmp(op1, op2) < 0) {
        apn_assign(rem, op1);
        apn_assign_dig(quot, 0); 
        return;
    }
    if(op2->_size < APN_DIV_BZ_THRESHOLD)
        apn_div_basecase(quot, rem, op1, op2);
    else
        apn_div_bz(quot, rem, op1, op2);
}

// long division
void apn_div_basecase(apn_s* dest_quot, apn_s* dest_rem, const apn_s* op1, const apn_s* op2) {
    apn_s quot, rem;
    apn_init_list(&quot, &rem, NULL);

    // msb to lsb
    size_t i = op1->_size - op2->_size; // start from position op2.size - 1
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

void apn_div_bz_d2n1n(apn_s* quot, apn_s* rem, const apn_s* op1, const apn_s* op2);
void apn_div_bz_d3n2n(apn_s* quot, apn_s* rem, const apn_s* op1, const apn_s* op2);

void apn_div_bz(apn_s* quot, apn_s* rem, const apn_s* op1, const apn_s* op2) {
//    if(op2->_size <= APN_DIV_BZ_THRESHOLD) {
//        apn_div_basecase(quot, rem, op1, op2);
//        return;
//    }
    // r-digit divide by s-digit number
    apn_s A, B, Z, Q, R; // A, B: copy of operands, Q, R: results, Z: temp
    apn_init_list(&A, &B, &Z, &Q, &R, NULL);
    apn_assign(&A, op1);
    apn_assign(&B, op2);
    // min{2^k | 2^k * BLOCKSIZE > s}, number of blocks to be divided
    size_t m = (B._size / APN_DIV_BZ_BLOCKSIZE);
    size_t t = m && !(m & (m - 1)); // is power of 2?
    while(m >>= 1)
        ++t;
    m = 1 << t;
    // n = ceil(s/m) * m, minimize block size to waste less 0. n > s.
    size_t n = (B._size / m + (bool)(B._size % m)) * m;
    // extend and normalize B, shift the same amount for A
    size_t sigmaQ = n - B._size, // shift amount for B, divided by base
           sigmaR = AP_DIG_BIT - ap_dig_msb(B._data[B._size - 1]) - 1; // leftover
    apn_shl(&B, &B, sigmaQ);
    apn_shl(&A, &A, sigmaQ);
    apn_bit_shl(&B, &B, sigmaR);
    apn_bit_shl(&A, &A, sigmaR);
    // find t = min {l >= 2 | A < base^(l*n) / 2},
    // Z store current beta^(l*n) / 2 for comparison.
    // (split A into t blocks of n digits, such that the most significant bit
    // of A[t-1] is zero. So the precondition for D2n/1n is satisfied.)
    apn_assign_dig(&Z, 1);
    apn_bit_shl(&Z, &Z, AP_DIG_BIT - 1); // base / 2
    apn_shl(&Z, &Z, 2 * n - 1);
    t = 2;
    while(apn_cmp(&A, &Z) >= 0) {
        apn_shl(&Z, &Z, n);
        ++t;
    }
    // main loop, like school division of division by 1 digit(block) using D2n/1n,
    // Z holds current partial remainder at start of loop.
    apn_assign_part_zero(&Z, &A, (t - 2) * n, 2 * n); // Initial Z = (A[t-1], A[t-2])
    size_t i = t - 2;
    do { // `d2n1n` precondition holds through
        apn_div_bz_d2n1n(&Z, &R, &Z, &B); // Z now hold the temp quotient,
        apn_shl(&Q, &Q, n); // by our choice of t, Q < base^n
        apn_add(&Q, &Q, &Z);
        if(i != 0) { // set Z = (R, A[i - 1]),
                     // D2n/1n precondition is still satisfied since R < B.
            apn_assign(&Z, &R);
            apn_shl(&Z, &Z, n);
            apn_assign_part_zero(&R, &A, (i - 1) * n, n); // R: temp, A[i - 1]
            apn_add(&Z, &Z, &R);
        }
    } while(i--);
    // shift remainder back / denormalize it
    apn_shr(&R, &R, sigmaQ);
    apn_bit_shr(&R, &R, sigmaR);

    if(quot != NULL)
        apn_swap(quot, &Q);
    if(rem != NULL)
        apn_swap(rem, &R);
    apn_clear_list(&A, &B, &Z, &Q, &R, NULL);
}

// Divide a 2n-digit number by a n-digit number, normalized so that
// base^n/2 <= B < base^n & A < base^n * B.
void apn_div_bz_d2n1n(apn_s* quot, apn_s* rem, const apn_s* op1, const apn_s* op2) {
    if(op2->_size & 1 || op2->_size < APN_DIV_BZ_THRESHOLD) {
        apn_div_basecase(quot, rem, op1, op2);
        return;
    }
    // split A into 4 parts, B into 2 parts with Ai, Bi < base^(n/2)
    apn_s Q, R, N; // Q, R: the results, D: dividend for D3n2n
    apn_init_list(&Q, &R, &N, NULL);
    size_t halfn = op2->_size / 2;
    // then Q1 = [A1, A2, A3] / [B1, B2]
    apn_assign_part_zero(&N, op1, halfn, halfn * 3);
    apn_div_bz_d3n2n(&Q, &R, &N, op2); // Q = Q1
    // Q2 = [R1, R2, A4] / [B1, B2]
    apn_assign(&N, &R);
    apn_shl(&N, &N, halfn);
    apn_assign_part_zero(&R, op1, 0, halfn); // R used to store A4
    apn_add(&N, &N, &R);
    apn_div_bz_d3n2n(&N, &R, &N, op2); // N = Q2
    // return results, Q = [Q1, Q2]
    apn_shl(&Q, &Q, op2->_size);
    apn_add(&Q, &Q, &N);
    
    if(quot != NULL)
        apn_swap(quot, &Q);
    if(rem != NULL)
        apn_swap(rem, &R);
    apn_clear_list(&Q, &R, &N, NULL);
}

// Divide 3n-digit number by a 2n-digit number, with operands normalized so
// base^(2n)/2 <= B < base^(2n), A < base^n * B.
void apn_div_bz_d3n2n(apn_s* quot, apn_s* rem, const apn_s* op1, const apn_s* op2) {
    // split A into 3 parts, B into 2 parts, each part < base^n
    apn_s Q, R, D, T;
    apn_init_list(&Q, &R, &D, &T, NULL);
    // if A1 < B1
    size_t n = op2->_size / 2;
    apn_assign_part_zero(&D, op1, 2 * n, n); // D = A1
    apn_assign_part_zero(&T, op2, n, n); // T = B1
    apn_assign_part_zero(&R, op1, n, 2 * n); // R = [A1, A2]

    if(apn_cmp(&D, &T) < 0)
        apn_div_bz_d2n1n(&Q, &R, &R, &T); // Q', R1 = [A1, A2] / B1, (approximate result)
    else {
        // Q' = base^n - 1
        apn_assign_dig(&Q, 1);
        apn_shl(&Q, &Q, n);
        apn_sub_dig(&Q, &Q, 1);
        // R1 = [A1, A2] - Q'B1 or [A1, A2] + [0, B1] - [B1, 0]
        apn_add(&R, &R, &T);
        apn_shl(&T, &T, n);
        apn_sub(&R, &R, &T);
    }
    // compute D = Q'B2
    apn_assign_part_zero(&T, op2, 0, n);
    apn_mul(&D, &Q, &T); // fast multiplication
    // compute R' = R1 * base^n + A4
    apn_shl(&R, &R, n);
    apn_assign_part_zero(&T, op1, 0, n);
    apn_add(&R, &R, &T);
    // on the paper was - D above and loop for R' < 0 here,
    // but this way signed integer is not needed.
    while(apn_cmp(&R, &D) < 0) {
        // R' += B, Q' -= 1
        apn_add(&R, &R, op2);
        apn_sub_dig(&Q, &Q, 1);
    }
    apn_sub(&R, &R, &D);
    // return results
    if(quot != NULL)
        apn_swap(quot, &Q);
    if(rem != NULL)
        apn_swap(rem, &R);
    apn_clear_list(&Q, &R, &D, &T);
}

