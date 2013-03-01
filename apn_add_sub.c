#include "apn.h"
#include "ap_impl.h"

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

