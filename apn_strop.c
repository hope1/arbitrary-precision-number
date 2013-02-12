#include "apn.h"
#include "ap_util.h"
#include <ctype.h>
#include <string.h>

static const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

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

    apn_s b, v, t;
    apn_init_list(&b, &v, &t, NULL);
    apn_assign_dig(&b, base);
    apn_assign(&v, o);

    char* p = str;
    do {
        apn_div(&v, &t, &v, &b);
        *p++ = alphabet[t._data[0]];
    } while(!apn_is_zero(&v));

    apn_clear_list(&b, &v, &t, NULL);

    *p-- = '\0';
    // the result was in reverse order
    while(p != str && p != str - 1) {
        Macro_swap_val(char, *p, *str);
        --p, ++str;
    }
}
