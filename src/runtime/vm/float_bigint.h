#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char* digits;     // Дигиты в виде строки
    int length;       // Длина числа (без знака и точки)
    int decimal_pos;  // Позиция десятичной точки от правого конца
    bool negative;    // Знак: true если отрицательное
    bool is_nan;      // true если NaN
    bool is_inf;      // true если бесконечность
} BigFloat;

// Функции для работы с BigFloat
BigFloat* bigfloat_create(const char* str);
void bigfloat_destroy(BigFloat* bf);
char* bigfloat_to_string(const BigFloat* bf);

// Арифметические операции
BigFloat* bigfloat_add(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_sub(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_mul(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_div(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_mod(const BigFloat* a, const BigFloat* b);
BigFloat* bigfloat_neg(const BigFloat* a);

// Сравнения
bool bigfloat_eq(const BigFloat* a, const BigFloat* b);
bool bigfloat_lt(const BigFloat* a, const BigFloat* b);
bool bigfloat_gt(const BigFloat* a, const BigFloat* b);
bool bigfloat_le(const BigFloat* a, const BigFloat* b);
bool bigfloat_ge(const BigFloat* a, const BigFloat* b);

// Константы
BigFloat* bigfloat_zero(void);
BigFloat* bigfloat_one(void);
BigFloat* bigfloat_nan(void);
BigFloat* bigfloat_inf(bool negative);
