#include "apn.h"
#include "apz.h"
#include "ap_impl.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void apn_realloc(apn_s* o, size_t new_capacity) {
    if(o->_size <= new_capacity)
        o->_data = realloc(o->_data, new_capacity * sizeof(ap_dig_t));
    else {
        free(o->_data);
        o->_data = calloc(new_capacity, sizeof(ap_dig_t));
        o->_size = 1;
    }
    o->_capacity = new_capacity;
}

void apn_data_fill_zero(apn_s* o) {
    memset(o->_data, 0, o->_capacity * sizeof(ap_dig_t));
}

void apn_init(apn_s* o) {
    o->_capacity = 1;
    o->_data = calloc(o->_capacity, sizeof(ap_dig_t));
    o->_size = 1;
}

void apn_init_list(apn_s* o, ...) {
    va_list args;
    va_start(args, o);
    while(o != NULL) {
        apn_init(o);
        o = va_arg(args, apn_s*);
    }
    va_end(args);
}

void apn_clear(apn_s* o) {
    free(o->_data);
    o->_data = NULL;
    o->_capacity = 0;
    o->_size = 0;
}

void apn_clear_list(apn_s* o, ...) {
    va_list args;
    va_start(args, o);
    while(o != NULL) {
        apn_clear(o);
        o = va_arg(args, apn_s*);
    }
    va_end(args);
}

void apn_swap(apn_s* a, apn_s* b) {
    Macro_swap_val(ap_dig_t*, a->_data, b->_data);
    Macro_swap_val(size_t, a->_size, b->_size);
    Macro_swap_val(size_t, a->_capacity, b->_capacity);
}

void apn_assign_part(apn_s* res, const apn_s* op, size_t start, size_t size) {
    assert(Macro_max(start, start + size) <= op->_size);
    if(!size) {
        apn_assign_dig(res, 0);
        return;
    }

    if(res->_capacity < size)
        apn_realloc(res, size);
    res->_size = size;
    memmove(res->_data, op->_data + start, size * sizeof(ap_dig_t));
}

void apn_assign_part_zero(apn_s* res, const apn_s* op, size_t start, size_t size) {
    // fills invalid ranges with 0
    if(start >= op->_size) {
        apn_assign_dig(res, 0);
        return;
    }
    if(start + size > op->_size) // partially invalid
        size = op->_size - start;

    apn_assign_part(res, op, start, size);
}

void apn_assign(apn_s* res, const apn_s* op) {
    apn_assign_part(res, op, 0, op->_size);
}

void apn_assign_dig(apn_s* o, ap_dig_t dig) {
    o->_size = 1;
    o->_data[0] = dig;
}

