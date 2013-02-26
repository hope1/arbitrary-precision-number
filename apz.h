#ifndef HOPE_BIGNUM_APZ_H
#define HOPE_BIGNUM_APZ_H

#include "apn.h"
#include <stddef.h>
#include <stdbool.h>

struct arbitrary_precision_integer {
    apn_s   magnitude;
    bool    sign; // false = positive
};
typedef struct arbitrary_precision_integer apz_s;

void apz_init(apz_s* o);
void apz_clear(apz_s* o);
void apz_init_list(apz_s* o, ...);
void apz_clear_list(apz_s* o, ...);

void apz_swap(apz_s* a, apz_s* b);
void apz_assign(apz_s* res, const apz_s* op);
void apz_assign_n(apz_s* res, const apn_s* op);

void apz_to_str(const apz_s* o, char* str, int base);

void apz_add(apz_s* res, const apz_s* op1, const apz_s* op2);
void apz_sub(apz_s* res, const apz_s* op1, const apz_s* op2);
void apz_mul(apz_s* res, const apz_s* op1, const apz_s* op2);
// remainder have same sign as dividend
void apz_div(apz_s* quot, apz_s* rem, const apz_s* op1, const apz_s* op2);
// modulo, guaranteed 0 <= `mod` < `op2`
void apz_mod(apz_s* mod, const apz_s* op1, const apz_s* op2);
void apz_mod_n(apn_s* mod, const apz_s* op1, const apn_s* op2);

#endif // HOPE_BIGNUM_APZ_H
