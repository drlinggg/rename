#ifndef FLOAT_BIGINT_H
#define FLOAT_BIGINT_H

#include <stdbool.h>

typedef struct {
    char* digits;
    int   len;
    int   decimal_pos;
    bool  neg;
    bool  is_nan;
    bool  is_inf;
} BigFloat;

BigFloat* bigfloat_create(const char* s);
void bigfloat_free(BigFloat* x);
void bigfloat_destroy(BigFloat* x);

char* bigfloat_to_string(const BigFloat* x);

BigFloat* bigfloat_add(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_sub(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_mul(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_div(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_mod(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_neg(const BigFloat* a);
BigFloat* bigfloat_sqrt(const BigFloat* a);

int bigfloat_cmp(const BigFloat* a, const BigFloat* b);
bool bigfloat_eq(const BigFloat* a, const BigFloat* b);
bool bigfloat_lt(const BigFloat* a, const BigFloat* b);
bool bigfloat_le(const BigFloat* a, const BigFloat* b);
bool bigfloat_gt(const BigFloat* a, const BigFloat* b);
bool bigfloat_ge(const BigFloat* a, const BigFloat* b);

BigFloat* bigfloat_zero(void);
BigFloat* bigfloat_one(void);
BigFloat* bigfloat_nan(void);
BigFloat* bigfloat_inf(bool negative);

#endif
