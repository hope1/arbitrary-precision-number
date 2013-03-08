#include "apn.h"
#include "ap_impl.h"

void apn_mul(apn_s* res, const apn_s* op1, const apn_s* op2) {
    size_t n = Macro_min(op1->_size, op2->_size);
    if(n <= APN_MUL_KARATSUBA_THRESHOLD)
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
    if(Macro_min(op1->_size, op2->_size) <= APN_MUL_KARATSUBA_THRESHOLD) { // base case
        apn_mul_basecase(res, op1, op2);
        return;
    }
    size_t m = Macro_max(op1->_size, op2->_size) / 2; // dividing point

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

