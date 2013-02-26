#include "apn.h"
#include <string.h>

void apn_shl(apn_s* res, const apn_s* o, size_t n) {
    if(!n)
        apn_assign(res, o);
    else if(!apn_is_zero(o)) {
        if(res->_capacity < o->_size + n)
            apn_realloc(res, o->_size + n);

        memmove(res->_data + n, o->_data, o->_size * sizeof(ap_dig_t));
        memset(res->_data, 0, n * sizeof(ap_dig_t));
        res->_size = o->_size + n;
    }
}

void apn_shr(apn_s* res, const apn_s* o, size_t n) {
    if(!n)
        return;

    if(o->_size <= n)
        apn_assign_dig(res, 0);
    else {
        size_t new_size = o->_size - n;
        if(res->_capacity < new_size)
            apn_realloc(res, new_size);

        memmove(res->_data, o->_data + n, new_size * sizeof(ap_dig_t));
        res->_size = new_size;
    }
}

void apn_bit_shl(apn_s* res, const apn_s* o, size_t n) {
    if(!n)
        return;

    if(res->_capacity < o->_size + 1)
        apn_realloc(res, o->_size + 1);

    ap_dig_t carry = 0;
    for(size_t i = 0; i != o->_size; ++i) {
        ap_dig_t x = o->_data[i];
        res->_data[i] = (x << n) + carry;
        carry = x >> (AP_DIG_BIT - n);
    }

    res->_size = o->_size;
    if(carry)
        res->_data[res->_size++] = carry;
}

void apn_bit_shr(apn_s* res, const apn_s* o, size_t n) {
    if(!n)
        return;

    if(res->_capacity < o->_size)
        apn_realloc(res, o->_size);

    ap_dig_t carry = 0;
    size_t i = o->_size - 1;
    do {
        ap_dig_t x = o->_data[i];
        res->_data[i] = (x >> n) + carry;
        carry = x << (AP_DIG_BIT - n);
    } while(i--);

    res->_size = o->_size;
    if(!res->_data[res->_size - 1] && res->_size != 1) // msb shifted out and result is not zero
        --res->_size;
}
