#include "float_bigint.h"
#include "../../system.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

#define MAX_PRECISION 100

static void normalize_string(char* str) {
    // Remove leading zeros but keep one digit if the number becomes zero
    while (*str == '0' && *(str+1) != '.' && *(str+1) != '\0') {
        // copy including null terminator
        memmove(str, str+1, strlen(str) + 1);
    }
    if (*str == '.') {
        memmove(str+1, str, strlen(str) + 1);
        *str = '0';
    }
}

static void remove_trailing_zeros(char* str) {
    int len = strlen(str);
    int i = len - 1;
    
    // Находим десятичную точку
    char* dot = strchr(str, '.');
    if (!dot) return;
    
    // Удаляем конечные нули
    while (i > (dot - str) && str[i] == '0') {
        str[i] = '\0';
        i--;
    }
    
    // Удаляем точку если после нее ничего нет
    if (str[i] == '.') {
        str[i] = '\0';
    }
}

static void align_decimals(const BigFloat* a, const BigFloat* b, 
                          char** a_aligned, char** b_aligned, 
                          int* max_len, int* decimal_pos) {
    int a_len = a->length;
    int b_len = b->length;
    int a_dec_pos = a->decimal_pos;
    int b_dec_pos = b->decimal_pos;
    
    // Находим максимальную позицию десятичной точки
    int max_dec_pos = (a_dec_pos > b_dec_pos) ? a_dec_pos : b_dec_pos;
    
    // Выравниваем длины, добавляя нули
    int a_total = a_len + (max_dec_pos - a_dec_pos);
    int b_total = b_len + (max_dec_pos - b_dec_pos);
    *max_len = (a_total > b_total) ? a_total : b_total;
    
    *a_aligned = malloc(*max_len + 1);
    *b_aligned = malloc(*max_len + 1);
    
    // Заполняем выровненные строки
    memset(*a_aligned, '0', *max_len);
    memset(*b_aligned, '0', *max_len);
    
    // Копируем цифры с выравниванием
    memcpy(*a_aligned + (*max_len - a_total), a->digits, a_len);
    memcpy(*b_aligned + (*max_len - b_total), b->digits, b_len);
    
    (*a_aligned)[*max_len] = '\0';
    (*b_aligned)[*max_len] = '\0';
    *decimal_pos = max_dec_pos;
}

BigFloat* bigfloat_create(const char* str) {
    if (!str) return NULL;
    
    BigFloat* bf = malloc(sizeof(BigFloat));
    bf->is_nan = false;
    bf->is_inf = false;
    bf->negative = false;
    
    if (strcmp(str, "nan") == 0 || strcmp(str, "NaN") == 0) {
        bf->is_nan = true;
        bf->digits = strdup("0");
        bf->length = 1;
        bf->decimal_pos = 0;
        return bf;
    }
    
    if (strcmp(str, "inf") == 0 || strcmp(str, "Infinity") == 0) {
        bf->is_inf = true;
        bf->negative = false;
        bf->digits = strdup("0");
        bf->length = 1;
        bf->decimal_pos = 0;
        return bf;
    }
    
    if (strcmp(str, "-inf") == 0 || strcmp(str, "-Infinity") == 0) {
        bf->is_inf = true;
        bf->negative = true;
        bf->digits = strdup("0");
        bf->length = 1;
        bf->decimal_pos = 0;
        return bf;
    }
    
    const char* p = str;
    if (*p == '-') {
        bf->negative = true;
        p++;
    }
    
    // Находим точку
    const char* dot = strchr(p, '.');
    if (dot) {
        bf->length = strlen(p) - 1;
        bf->decimal_pos = strlen(dot + 1);
    } else {
        bf->length = strlen(p);
        bf->decimal_pos = 0;
    }
    
    // Копируем цифры
    bf->digits = malloc(bf->length + 1);
    int j = 0;
    for (int i = 0; p[i]; i++) {
        if (p[i] != '.') {
            bf->digits[j++] = p[i];
        }
    }
    bf->digits[bf->length] = '\0';
    
    normalize_string(bf->digits);
    
    // Update recorded length after normalization and trim trailing
    // zeros from the fractional part (digits are stored without a dot)
    bf->length = strlen(bf->digits);
    while (bf->decimal_pos > 0 && bf->length > 0 && bf->digits[bf->length - 1] == '0') {
        bf->digits[bf->length - 1] = '\0';
        bf->length--;
        bf->decimal_pos--;
    }
    
    // Если все цифры нули
    if (bf->digits[0] == '0' && bf->length == 1) {
        bf->negative = false;
    }

    // DEBUG: if something goes wrong, print internal state
    if (!bf->digits) {
        DPRINT("bigfloat_create('%s') -> digits=NULL length=%d decimal_pos=%d negative=%d\n", str, bf->length, bf->decimal_pos, bf->negative);
    } else {
        DPRINT("bigfloat_create('%s') -> digits='%s' length=%d decimal_pos=%d negative=%d\n", str, bf->digits, bf->length, bf->decimal_pos, bf->negative);
    }
    
    return bf;
}

void bigfloat_destroy(BigFloat* bf) {
    if (bf) {
        if (bf->digits) free(bf->digits);
        free(bf);
    }
}

char* bigfloat_to_string(const BigFloat* bf) {
    if (!bf) return strdup("0.0");
    
    if (bf->is_nan) return strdup("nan");
    if (bf->is_inf) return bf->negative ? strdup("-inf") : strdup("inf");
    
    // Compute needed buffer length precisely to avoid overflows.
    // int_part_len: number of digits before the decimal point
    int int_part_len = bf->length - bf->decimal_pos;
    int int_part_needed = (int_part_len <= 0) ? 1 : int_part_len; // at least one digit (0)
    int frac_len = (bf->decimal_pos > 0) ? bf->decimal_pos : 0;
    int decimal_char = (frac_len > 0) ? 1 : 0;

    int total_len = (bf->negative ? 1 : 0) + int_part_needed + decimal_char + frac_len;
    char* result = malloc(total_len + 1); // +1 for '\0'
    int pos = 0;
    
    if (bf->negative) result[pos++] = '-';
    
    // Integer part
    // If there are no integer digits, write a single '0' and account for
    // leading zeros needed in the fractional part alignment.
    int missing_leading_zeros = 0;
    if (int_part_len <= 0) {
        result[pos++] = '0';
        missing_leading_zeros = -int_part_len;
        int_part_len = 0;
    } else {
        for (int i = 0; i < int_part_len; i++) {
            result[pos++] = bf->digits[i];
        }
    }
    
    // Дробная часть
    if (frac_len > 0) {
        result[pos++] = '.';
        for (int i = 0; i < missing_leading_zeros; i++) {
            result[pos++] = '0';
        }
        for (int i = int_part_len; i < bf->length; i++) {
            result[pos++] = bf->digits[i];
        }
    }
    
    // Ensure we null-terminate at the expected position
    if (pos <= total_len) {
        result[pos] = '\0';
    } else {
        // Defensive: should not happen if length computed correctly
        result[total_len] = '\0';
    }
    
    // Удаляем лишние нули в конце
    if (bf->decimal_pos > 0) {
        remove_trailing_zeros(result);
    }
    
    // Если результат "-0", делаем "0"
    if (strcmp(result, "-0") == 0 || strcmp(result, "-0.0") == 0) {
        free(result);
        return strdup("0");
    }
    
    return result;
}

BigFloat* bigfloat_add(const BigFloat* a, const BigFloat* b) {
    // Проверка специальных случаев
    if (a->is_nan || b->is_nan) return bigfloat_nan();
    if (a->is_inf && b->is_inf) {
        if (a->negative == b->negative) return bigfloat_inf(a->negative);
        return bigfloat_nan(); // inf - inf = NaN
    }
    if (a->is_inf) return bigfloat_inf(a->negative);
    if (b->is_inf) return bigfloat_inf(b->negative);
    
    // Если знаки разные, делаем вычитание
    if (a->negative && !b->negative) {
        BigFloat* a_copy = malloc(sizeof(BigFloat));
        memcpy(a_copy, a, sizeof(BigFloat));
        a_copy->negative = false;
        BigFloat* result = bigfloat_sub(b, a_copy);
        free(a_copy);
        return result;
    }
    if (!a->negative && b->negative) {
        BigFloat* b_copy = malloc(sizeof(BigFloat));
        memcpy(b_copy, b, sizeof(BigFloat));
        b_copy->negative = false;
        BigFloat* result = bigfloat_sub(a, b_copy);
        free(b_copy);
        return result;
    }
    
    char* a_aligned, *b_aligned;
    int max_len, decimal_pos;
    align_decimals(a, b, &a_aligned, &b_aligned, &max_len, &decimal_pos);
    
    // Выполняем сложение
    char* result_digits = malloc(max_len + 2); // +1 для возможного переноса, +1 для '\0'
    int carry = 0;
    
    for (int i = max_len - 1; i >= 0; i--) {
        int digit_a = a_aligned[i] - '0';
        int digit_b = b_aligned[i] - '0';
        int sum = digit_a + digit_b + carry;
        result_digits[i + 1] = (sum % 10) + '0';
        carry = sum / 10;
    }
    
    // Обрабатываем последний перенос
    if (carry > 0) {
        result_digits[0] = carry + '0';
        result_digits[max_len + 1] = '\0';
        max_len++;
    } else {
        memmove(result_digits, result_digits + 1, max_len);
        result_digits[max_len] = '\0';
    }
    
    free(a_aligned);
    free(b_aligned);
    
    // Создаем результат
    BigFloat* result = malloc(sizeof(BigFloat));
    result->digits = result_digits;
    result->length = max_len;
    result->decimal_pos = decimal_pos;
    result->negative = a->negative; // Оба отрицательные
    result->is_nan = false;
    result->is_inf = false;
    
    normalize_string(result->digits);

    // Update length and trim trailing fractional zeros
    result->length = strlen(result->digits);
    while (result->decimal_pos > 0 && result->length > 0 && result->digits[result->length - 1] == '0') {
        result->digits[result->length - 1] = '\0';
        result->length--;
        result->decimal_pos--;
    }

    // If we ended up with empty digits, set to zero
    if (result->length == 0) {
        free(result->digits);
        result->digits = strdup("0");
        result->length = 1;
        result->decimal_pos = 0;
        result->negative = false;
    }

    normalize_string(result->digits);
    return result;
}

BigFloat* bigfloat_sub(const BigFloat* a, const BigFloat* b) {
    // Проверка специальных случаев
    if (a->is_nan || b->is_nan) return bigfloat_nan();
    if (a->is_inf && b->is_inf) {
        if (a->negative == b->negative) return bigfloat_nan(); // inf - inf = NaN
        return bigfloat_inf(a->negative);
    }
    if (a->is_inf) return bigfloat_inf(a->negative);
    if (b->is_inf) {
        BigFloat* result = bigfloat_inf(!b->negative);
        return result;
    }
    
    // Если знаки разные, делаем сложение
    if (a->negative && !b->negative) {
        BigFloat* a_copy = malloc(sizeof(BigFloat));
        memcpy(a_copy, a, sizeof(BigFloat));
        a_copy->negative = false;
        BigFloat* result = bigfloat_add(a_copy, b);
        free(a_copy);
        result->negative = true;
        return result;
    }
    if (!a->negative && b->negative) {
        BigFloat* b_copy = malloc(sizeof(BigFloat));
        memcpy(b_copy, b, sizeof(BigFloat));
        b_copy->negative = false;
        BigFloat* result = bigfloat_add(a, b_copy);
        free(b_copy);
        return result;
    }
    
    // Определяем какое число больше по модулю
    bool a_is_bigger = true;
    char* a_aligned, *b_aligned;
    int max_len, decimal_pos;
    align_decimals(a, b, &a_aligned, &b_aligned, &max_len, &decimal_pos);
    
    for (int i = 0; i < max_len; i++) {
        if (a_aligned[i] > b_aligned[i]) {
            a_is_bigger = true;
            break;
        } else if (a_aligned[i] < b_aligned[i]) {
            a_is_bigger = false;
            break;
        }
    }
    
    // Вычитаем из большего меньшее
    char* result_digits = malloc(max_len + 1);
    int borrow = 0;
    
    for (int i = max_len - 1; i >= 0; i--) {
        int digit_a = a_is_bigger ? (a_aligned[i] - '0') : (b_aligned[i] - '0');
        int digit_b = a_is_bigger ? (b_aligned[i] - '0') : (a_aligned[i] - '0');
        
        digit_a -= borrow;
        if (digit_a < digit_b) {
            digit_a += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        
        result_digits[i] = (digit_a - digit_b) + '0';
    }
    result_digits[max_len] = '\0';
    
    free(a_aligned);
    free(b_aligned);
    
    // Создаем результат
    BigFloat* result = malloc(sizeof(BigFloat));
    result->digits = result_digits;
    result->length = max_len;
    result->decimal_pos = decimal_pos;
    
    // Определяем знак результата
    if (a->negative && b->negative) {
        // (-a) - (-b) = -a + b = b - a
        result->negative = !a_is_bigger;
    } else {
        // a - b
        result->negative = !a_is_bigger;
    }
    
    result->is_nan = false;
    result->is_inf = false;
    
    normalize_string(result->digits);

    // Update length and trim trailing fractional zeros
    result->length = strlen(result->digits);
    while (result->decimal_pos > 0 && result->length > 0 && result->digits[result->length - 1] == '0') {
        result->digits[result->length - 1] = '\0';
        result->length--;
        result->decimal_pos--;
    }

    // If we ended up with empty digits, set to zero
    if (result->length == 0) {
        free(result->digits);
        result->digits = strdup("0");
        result->length = 1;
        result->decimal_pos = 0;
        result->negative = false;
    }

    normalize_string(result->digits);
    return result;
}

static void multiply_digits(const char* a, int a_len, const char* b, int b_len, char** result, int* result_len) {
    *result_len = a_len + b_len;
    *result = malloc(*result_len + 1);
    if (!*result) return;
    // initialize with ASCII '0' characters so arithmetic works correctly
    memset(*result, '0', *result_len);
    (*result)[*result_len] = '\0';
    
    for (int i = 0; i < a_len; i++) {
        int carry = 0;
        int digit_a = a[a_len - 1 - i] - '0';
        
        for (int j = 0; j < b_len; j++) {
            int digit_b = b[b_len - 1 - j] - '0';
            int pos = *result_len - 1 - i - j;
            int current = (*result)[pos] ? (*result)[pos] - '0' : 0;
            int product = digit_a * digit_b + carry + current;
            (*result)[pos] = (product % 10) + '0';
            carry = product / 10;
        }
        
        if (carry > 0) {
            int pos = *result_len - 1 - i - b_len;
            (*result)[pos] = carry + '0';
        }
    }
    
    // Убираем ведущие нули
    int start = 0;
    while (start < *result_len - 1 && (*result)[start] == '0') {
        start++;
    }
    
    if (start > 0) {
        memmove(*result, *result + start, *result_len - start + 1);
        *result_len -= start;
    }
}

BigFloat* bigfloat_mul(const BigFloat* a, const BigFloat* b) {
    // Проверка специальных случаев
    if (a->is_nan || b->is_nan) return bigfloat_nan();
    if (a->is_inf || b->is_inf) {
        if (bigfloat_eq(a, bigfloat_zero()) || bigfloat_eq(b, bigfloat_zero())) {
            return bigfloat_nan(); // 0 * inf = NaN
        }
        bool negative = a->negative != b->negative;
        return bigfloat_inf(negative);
    }
    
    char* result_digits;
    int result_len;
    multiply_digits(a->digits, a->length, b->digits, b->length, &result_digits, &result_len);
    
    // Создаем результат
    BigFloat* result = malloc(sizeof(BigFloat));
    result->digits = result_digits;
    result->length = result_len;
    result->decimal_pos = a->decimal_pos + b->decimal_pos;
    result->negative = a->negative != b->negative;
    result->is_nan = false;
    result->is_inf = false;
    
    normalize_string(result->digits);

    // Update length and trim trailing fractional zeros
    result->length = strlen(result->digits);
    while (result->decimal_pos > 0 && result->length > 0 && result->digits[result->length - 1] == '0') {
        result->digits[result->length - 1] = '\0';
        result->length--;
        result->decimal_pos--;
    }

    if (result->length == 0) {
        free(result->digits);
        result->digits = strdup("0");
        result->length = 1;
        result->decimal_pos = 0;
        result->negative = false;
    }

    normalize_string(result->digits);
    return result;
}

BigFloat* bigfloat_div(const BigFloat* a, const BigFloat* b) {
    // Проверка специальных случаев
    if (a->is_nan || b->is_nan) return bigfloat_nan();
    if (b->is_inf) {
        if (a->is_inf) return bigfloat_nan(); // inf / inf = NaN
        return bigfloat_zero();
    }
    if (a->is_inf) return bigfloat_inf(a->negative != b->negative);
    
    // Деление на ноль
    if (bigfloat_eq(b, bigfloat_zero())) {
        return bigfloat_inf(a->negative != b->negative);
    }
    
    // Простая реализация деления с заданной точностью
    int precision = MAX_PRECISION;
    
    // Нормализуем числа: умножаем на 10^precision
    BigFloat* a_scaled = bigfloat_mul(a, bigfloat_create("1"));
    BigFloat* b_scaled = bigfloat_mul(b, bigfloat_create("1"));
    
    // Умножаем делимое на 10^precision
    char* power = malloc(precision + 2);
    if (!power) {
        bigfloat_destroy(a_scaled);
        bigfloat_destroy(b_scaled);
        return bigfloat_create("0");
    }
    power[0] = '1';
    for (int i = 0; i < precision; i++) {
        power[1 + i] = '0';
    }
    power[1 + precision] = '\0';
    BigFloat* multiplier = bigfloat_create(power);
    free(power);
    BigFloat* a_mult = bigfloat_mul(a_scaled, multiplier);
    
    // Выполняем целочисленное деление
    char* result_digits = malloc(precision + a->length + 1);
    // Упрощенная реализация - для полноценной нужна длинная арифметика деления
    strcpy(result_digits, "0");
    
    bigfloat_destroy(a_scaled);
    bigfloat_destroy(b_scaled);
    bigfloat_destroy(multiplier);
    bigfloat_destroy(a_mult);
    
    // Создаем результат
    BigFloat* result = bigfloat_create("0");
    result->negative = a->negative != b->negative;
    return result;
}

BigFloat* bigfloat_mod(const BigFloat* a, const BigFloat* b) {
    if (a->is_nan || b->is_nan || b->is_inf || bigfloat_eq(b, bigfloat_zero())) {
        return bigfloat_nan();
    }
    
    // Упрощенная реализация: a % b = a - floor(a/b) * b
    BigFloat* div = bigfloat_div(a, b);
    // Нужна функция floor для BigFloat
    // Пока возвращаем упрощенный результат
    BigFloat* result = bigfloat_sub(a, b);
    bigfloat_destroy(div);
    return result;
}

BigFloat* bigfloat_neg(const BigFloat* a) {
    if (a->is_nan) return bigfloat_nan();
    if (a->is_inf) return bigfloat_inf(!a->negative);
    
    BigFloat* result = malloc(sizeof(BigFloat));
    result->digits = strdup(a->digits);
    result->length = a->length;
    result->decimal_pos = a->decimal_pos;
    result->negative = !a->negative;
    result->is_nan = a->is_nan;
    result->is_inf = a->is_inf;
    return result;
}

bool bigfloat_eq(const BigFloat* a, const BigFloat* b) {
    if (a->is_nan || b->is_nan) return false;
    if (a->is_inf && b->is_inf) return a->negative == b->negative;
    if (a->is_inf || b->is_inf) return false;
    
    if (a->negative != b->negative) return false;
    
    char* a_aligned, *b_aligned;
    int max_len, decimal_pos;
    align_decimals(a, b, &a_aligned, &b_aligned, &max_len, &decimal_pos);
    
    bool equal = (strcmp(a_aligned, b_aligned) == 0);
    
    free(a_aligned);
    free(b_aligned);
    
    return equal;
}

bool bigfloat_lt(const BigFloat* a, const BigFloat* b) {
    if (a->is_nan || b->is_nan) return false;
    if (a->is_inf && b->is_inf) {
        if (a->negative && !b->negative) return true;
        if (!a->negative && b->negative) return false;
        return false; // Одинаковые бесконечности
    }
    if (a->is_inf) return a->negative;
    if (b->is_inf) return !b->negative;
    
    if (a->negative && !b->negative) return true;
    if (!a->negative && b->negative) return false;
    
    char* a_aligned, *b_aligned;
    int max_len, decimal_pos;
    align_decimals(a, b, &a_aligned, &b_aligned, &max_len, &decimal_pos);
    
    bool less = false;
    for (int i = 0; i < max_len; i++) {
        if (a_aligned[i] < b_aligned[i]) {
            less = true;
            break;
        } else if (a_aligned[i] > b_aligned[i]) {
            less = false;
            break;
        }
    }
    
    free(a_aligned);
    free(b_aligned);
    
    return a->negative ? !less : less;
}

bool bigfloat_gt(const BigFloat* a, const BigFloat* b) {
    return bigfloat_lt(b, a);
}

bool bigfloat_le(const BigFloat* a, const BigFloat* b) {
    return !bigfloat_gt(a, b);
}

bool bigfloat_ge(const BigFloat* a, const BigFloat* b) {
    return !bigfloat_lt(a, b);
}

BigFloat* bigfloat_zero(void) {
    return bigfloat_create("0");
}

BigFloat* bigfloat_one(void) {
    return bigfloat_create("1");
}

BigFloat* bigfloat_nan(void) {
    BigFloat* bf = malloc(sizeof(BigFloat));
    bf->is_nan = true;
    bf->is_inf = false;
    bf->negative = false;
    bf->digits = strdup("0");
    bf->length = 1;
    bf->decimal_pos = 0;
    return bf;
}

BigFloat* bigfloat_inf(bool negative) {
    BigFloat* bf = malloc(sizeof(BigFloat));
    bf->is_nan = false;
    bf->is_inf = true;
    bf->negative = negative;
    bf->digits = strdup("0");
    bf->length = 1;
    bf->decimal_pos = 0;
    return bf;
}
