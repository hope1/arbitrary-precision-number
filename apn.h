#ifndef HOPE_BIGNUM_APN_H
#define HOPE_BIGNUM_APN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint64_t ap_dig_t;
#define AP_DIG_MAX UINT64_MAX
#define AP_DIG_BIT 64 // 2^n

struct arbitrary_precision_natural {
    ap_dig_t* _data; // little-endian
    size_t    _capacity; // allocated size
    size_t    _size; // valid digits
};
typedef struct arbitrary_precision_natural apn_s;

#define APN_MUL_KARATSUBA_THRESHOLD 50
#define APN_DIV_BZ_THRESHOLD        64
#define APN_DIV_BZ_BLOCKSIZE        64

void apn_init(apn_s* o);
void apn_clear(apn_s* o);
// NULL-terminated
void apn_init_list(apn_s* o, ...);
void apn_clear_list(apn_s* o, ...);

void apn_swap(apn_s* a, apn_s* b);
void apn_realloc(apn_s* o, size_t new_capacity);
void apn_assign(apn_s* res, const apn_s* op);
// asserts valid range
void apn_assign_part(apn_s* res, const apn_s* op, size_t start, size_t size);
// fill invalid range with zero
void apn_assign_part_zero(apn_s* res, const apn_s* op, size_t start, size_t size);

void apn_assign_dig(apn_s* o, ap_dig_t dig);
// 2 <= base <= 36
void apn_assign_str(apn_s* o, const char* str, int base);
void apn_to_str(const apn_s* o, char* str, int base);
void apn_to_str_bexp2(const apn_s* o, char* str, int blog2);

bool apn_is_zero(const apn_s* o);
bool apn_is_odd(const apn_s* o);

// shift digits
void apn_shl(apn_s* res, const apn_s* o, size_t n);
void apn_shr(apn_s* res, const apn_s* o, size_t n);
// bitwise shift, n < AP_DIG_BIT
void apn_bit_shl(apn_s* res, const apn_s* o, size_t n);
void apn_bit_shr(apn_s* res, const apn_s* o, size_t n);

int apn_cmp(const apn_s* op1, const apn_s* op2); // stdlib-style compare function
int apn_cmp_dig(const apn_s* op, ap_dig_t dig);

void apn_add(apn_s* res, const apn_s* op1, const apn_s* op2);
void apn_add_dig(apn_s* res, const apn_s* op, ap_dig_t dig);
// op2 < op1
void apn_sub(apn_s* res, const apn_s* op1, const apn_s* op2);
void apn_sub_dig(apn_s* res, const apn_s* op, ap_dig_t dig);

void apn_mul(apn_s* res, const apn_s* op1, const apn_s* op2);
void apn_mul_basecase(apn_s* res, const apn_s* op1, const apn_s* op2);
void apn_mul_karatsuba(apn_s* res, const apn_s* op1, const apn_s* op2);
// quot and rem can be NULL.
void apn_div(apn_s* quot, apn_s* rem, const apn_s* op1, const apn_s* op2);
void apn_div_basecase(apn_s* quot, apn_s* rem, const apn_s* op1, const apn_s* op2);
void apn_div_bz(apn_s* quot, apn_s* rem, const apn_s* op1, const apn_s* op2); // TODO
// [op1 / op2]
void apn_idiv(apn_s* quot, const apn_s* op1, const apn_s* op2); // TODO

void apn_sqr(apn_s* res, const apn_s* op);
void apn_exp(apn_s* res, const apn_s* base, const apn_s* exp);
void apn_exp_dig(apn_s* res, const apn_s* base, ap_dig_t exp);
void apn_exp_bysqr(apn_s* res, const apn_s* base, const apn_s* exp); // exp != 0
void apn_modexp(apn_s* res, const apn_s* base, const apn_s* exp, const apn_s* mod);

void apn_gcd(apn_s* res, const apn_s* op1, const apn_s* op2);
// apn_gcd(a, mod) = 1, or `a` and `mod` are coprime
void apn_modinv(apn_s* inv, const apn_s* a, const apn_s* mod);

// low level
void apn_data_fill_zero(apn_s* o);
size_t apn_data_add(ap_dig_t* rp, const ap_dig_t* ap, size_t an,
                    const ap_dig_t* bp, size_t bn);
size_t apn_data_sub(ap_dig_t* rp, const ap_dig_t* ap, size_t an,
                    const ap_dig_t* bp, size_t bn);

#endif // HOPE_BIGNUM_APN_H
