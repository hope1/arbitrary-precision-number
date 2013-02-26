#include "apn.h"

bool apn_is_zero(const apn_s* o) {
    return !o->_data[o->_size - 1];
}

bool apn_is_odd(const apn_s* o) {
    return o->_data[0] & 1;
}

int apn_cmp(const apn_s* op1, const apn_s* op2) {
    if(op1->_size != op2->_size)
        return op1->_size < op2->_size ? -1 : 1;

    size_t i = op1->_size - 1;
    do
        if(op1->_data[i] != op2->_data[i])
            return op1->_data[i] < op2->_data[i] ? -1 : 1;
    while(i--);
    // equal
    return 0;
}

int apn_cmp_dig(const apn_s* op, ap_dig_t dig) {
    if(op->_size != 1)
        return 1;
    if(op->_data[0] == dig)
        return 0;
    return op->_data[0] < dig ? -1 : 1;
}
