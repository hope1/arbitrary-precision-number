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
    apn_div_basecase(dest_quot, dest_rem, op1, op2);
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

