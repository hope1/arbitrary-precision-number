#include "apz.h"
#include "ap_util.h"
#include <stdarg.h>

void apz_init(apz_s* o) {
    apn_init(&o->magnitude);
    o->sign = false;
}

void apz_clear(apz_s* o) {
    apn_clear(&o->magnitude);
    o->sign = false;
}

void apz_init_list(apz_s* o, ...) {
    va_list args;
    va_start(args, o);
    while(o != NULL) {
        apz_init(o);
        o = va_arg(args, apz_s*);
    }
    va_end(args);
}

void apz_clear_list(apz_s* o, ...) {
    va_list args;
    va_start(args, o);
    while(o != NULL) {
        apz_clear(o);
        o = va_arg(args, apz_s*);
    }
    va_end(args);
}

void apz_swap(apz_s* a, apz_s* b) {
    apn_swap(&a->magnitude, &b->magnitude);
    Macro_swap_val(bool, a->sign, b->sign);
}

void apz_assign(apz_s* res, const apz_s* op) {
    apn_assign(&res->magnitude, &op->magnitude);
    res->sign = op->sign;
}

void apz_assign_n(apz_s* res, const apn_s* op) {
    apn_assign(&res->magnitude, op);
    res->sign = false;
}

void apz_to_str(const apz_s* o, char* str, int base) {
    if(o->sign)
        *str++ = '-';
    apn_to_str(&o->magnitude, str, base);
}

static inline void apz_add_impl(apz_s* res, const apz_s* op1, const apz_s* op2, bool inv) {
    if(op1->sign == (inv ^ op2->sign)) { // inv: invert the sign of op2 for subtraction
        apn_add(&res->magnitude, &op1->magnitude, &op2->magnitude);
        res->sign = op1->sign;
    } else { // different sign
        bool f = (apn_cmp(&op1->magnitude, &op2->magnitude) < 0);
        apn_sub(&res->magnitude, f ? &op2->magnitude : &op1->magnitude,
                            f ? &op1->magnitude : &op2->magnitude);
        res->sign = f ^ (op1->sign);
    }
}

void apz_add(apz_s* res, const apz_s* op1, const apz_s* op2) {
    apz_add_impl(res, op1, op2, false);
}

void apz_sub(apz_s* res, const apz_s* op1, const apz_s* op2) {
    apz_add_impl(res, op1, op2, true);
}

void apz_mul(apz_s* res, const apz_s* op1, const apz_s* op2) {
    apn_mul(&res->magnitude, &op1->magnitude, &op2->magnitude);
    res->sign = op1->sign ^ op2->sign;
}

void apz_div(apz_s* quot, apz_s* rem, const apz_s* op1, const apz_s* op2) {
    apn_div(&quot->magnitude, &rem->magnitude, &op1->magnitude, &op2->magnitude);
    quot->sign = op1->sign ^ op2->sign;
    rem->sign = op1->sign; // same sign with dividend
}

void apz_mod(apz_s* mod, const apz_s* op1, const apz_s* op2) {
    apz_div(NULL, mod, op1, op2);
    if(mod->sign)
        apz_add(mod, mod, op2);
}

void apz_mod_n(apn_s* mod, const apz_s* op1, const apn_s* op2) {
    apn_div(NULL, mod, &op1->magnitude, op2);
    if(op1->sign)
        apn_sub(mod, op2, mod);
}
