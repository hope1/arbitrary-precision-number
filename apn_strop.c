#include "apn.h"
#include "ap_util.h"
#include <ctype.h>
#include <string.h>

static const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static ap_dig_t max_power[35][2]; // max i^x <= 2^AP_DIG_BIT, {i^x - 1, x} is stored

void precompute_table_max_power(void) {
    for(int i = 2; i <= 36; ++i) {
        struct ap_dig_pair t = { .lo = i, .hi = 0 };
        ap_dig_t x = 1;
        while(!t.hi) {
            t = ap_dig_mul(t.lo, i);
            ++x;
        }
        if(!t.lo && t.hi == 1)
            max_power[i - 2][0] = AP_DIG_MAX;
        else { // back to last step
            max_power[i - 2][0] = ap_dig_div_2d1t1(t, i) - 1;
            --x;
        }
        max_power[i - 2][1] = x;
    }
}

void apn_assign_str(apn_s* o, const char* str, int base) {
    if(base < 2 || base > 36)
        return;

    apn_s b;
    apn_init(&b);
    apn_assign_dig(&b, base);

    apn_assign_dig(o, 0);
    for(; *str; ++str) {
        apn_mul(o, o, &b);
        apn_add_dig(o, o, strchr(alphabet, toupper(*str)) - alphabet);
    }
    apn_clear(&b);
}

void apn_to_str_bexp2(const apn_s* o, char* str, int blog2) {
    if(blog2 < 1 || blog2 > 5)
        return;

    apn_s v, t, u;
    apn_init_list(&v, &t, &u, NULL);
    apn_assign(&v, o);

    char* p = str;
    do {
        apn_assign(&t, &v);
        apn_bit_shr(&v, &v, blog2);
        apn_bit_shl(&u, &v, blog2);
        apn_sub(&t, &t, &u);
        *p++ = alphabet[t._data[0]];
    } while(!apn_is_zero(&v));

    apn_clear_list(&v, &t, &u, NULL);

    *p-- = '\0';
    while(p != str && p != str - 1) {
        Macro_swap_val(char, *p, *str);
        --p, ++str;
    }
}

void apn_to_str(const apn_s* o, char* str, int base) {
    if(base < 2 || base > 36)
        return;

    if(!max_power[0][1])
        precompute_table_max_power();

    apn_s b, v, t;
    apn_init_list(&b, &v, &t, NULL);
    apn_assign(&v, o);
    // divide max_power(base) instead of one base digit at a time
    apn_assign_dig(&b, max_power[base - 2][0]);
    apn_add_dig(&b, &b, 1);
    
    char* p = str;
    ap_dig_t x = max_power[base - 2][1];
    do {
        apn_div(&v, &t, &v, &b);
        // t < max_power[base - 2] + 1 <= 2^AP_DIG_BIT
        ap_dig_t m = t._data[0];
        
        bool last_iter = (apn_cmp(&v, &b) < 0); // throw preceding zeros
        for(int i = 0; last_iter ? m : i != x; ++i) {
            *p++ = alphabet[m % base];
            m /= base;
        }
    } while(!apn_is_zero(&v));

    apn_clear_list(&b, &v, &t, NULL);

    *p-- = '\0';
    // the result was in reverse order
    while(p != str && p != str - 1) {
        Macro_swap_val(char, *p, *str);
        --p, ++str;
    }
}
