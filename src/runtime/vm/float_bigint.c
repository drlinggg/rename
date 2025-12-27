#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "float_bigint.h"

#define BF_PRECISION 25
#define BF_SQRT_ITERS 200


static int bigfloat_cmp_abs(const BigFloat* a, const BigFloat* b);
static BigFloat* bf_new(void) {
    BigFloat* x = calloc(1, sizeof(BigFloat));
    x->digits = strdup("0");
    x->len = 1;
    return x;
}

static BigFloat* bf_zero(void) { return bf_new(); }

static BigFloat* bf_one(void) {
    BigFloat* x = bf_new();
    free(x->digits);
    x->digits = strdup("1");
    x->len = 1;
    return x;
}

static BigFloat* bf_nan(void) {
    BigFloat* x = bf_new();
    x->is_nan = true;
    return x;
}

static BigFloat* bf_inf(bool neg) {
    BigFloat* x = bf_new();
    x->is_inf = true;
    x->neg = neg;
    return x;
}

void bigfloat_free(BigFloat* x) {
    if (!x) return;
    free(x->digits);
    free(x);
}

void bigfloat_destroy(BigFloat* x) { bigfloat_free(x); }


static void bf_trim(BigFloat* x) {
    if (x->is_nan || x->is_inf) return;

    // Убираем лишние нули справа (дробная часть)
    while (x->decimal_pos > 0 && x->len > 1 && x->digits[x->len - 1] == '0') {
        x->len--;
        x->decimal_pos--;
        x->digits[x->len] = 0;
    }

    // Убираем лишние нули слева (целая часть)
    int i = 0;
    while (i < x->len - 1 && x->digits[i] == '0' && x->len - i > x->decimal_pos) i++;
    if (i) {
        memmove(x->digits, x->digits + i, x->len - i);
        x->len -= i;
        x->digits[x->len] = 0;
    }

    // Если число = 0
    if (x->len == 1 && x->digits[0] == '0') {
        x->decimal_pos = 0;
        x->neg = false;
    }
}

static void bf_limit_precision(BigFloat* x, int max_decimal_digits) {
    if (x->is_nan || x->is_inf) return;
    if (x->decimal_pos <= max_decimal_digits) return;
    
    // Сколько цифр нужно отрезать
    int digits_to_remove = x->decimal_pos - max_decimal_digits;
    
    if (digits_to_remove <= 0) return;
    if (digits_to_remove >= x->len) {
        // Отрезаем все - результат 0
        free(x->digits);
        x->digits = strdup("0");
        x->len = 1;
        x->decimal_pos = 0;
        x->neg = false;
        return;
    }
    
    // Отрезаем с конца, но нужно округление
    int cut_pos = x->len - digits_to_remove;
    
    // Проверяем цифру после cut_pos для округления
    if (cut_pos > 0 && cut_pos < x->len) {
        char next_digit = x->digits[cut_pos];
        
        // Обрезаем
        x->len = cut_pos;
        x->digits[x->len] = '\0';
        x->decimal_pos = max_decimal_digits;
        
        // Округляем вверх если следующая цифра >= 5
        if (next_digit >= '5') {
            // Нужно увеличить последнюю цифру
            int i = x->len - 1;
            while (i >= 0) {
                if (x->digits[i] < '9') {
                    x->digits[i]++;
                    break;
                } else {
                    x->digits[i] = '0';
                    i--;
                }
            }
            
            // Если все цифры стали 9 -> 10...
            if (i < 0) {
                // Было 999... -> 1000...
                char* new_digits = malloc(x->len + 2);
                new_digits[0] = '1';
                memset(new_digits + 1, '0', x->len);
                new_digits[x->len + 1] = '\0';
                free(x->digits);
                x->digits = new_digits;
                x->len++;
            }
        }
    }
    
    bf_trim(x);
}



static BigFloat* bf_copy(const BigFloat* a) {
    BigFloat* x = bf_new();
    free(x->digits);
    x->digits = strdup(a->digits);
    x->len = a->len;
    x->decimal_pos = a->decimal_pos;
    x->neg = a->neg;
    x->is_nan = a->is_nan;
    x->is_inf = a->is_inf;
    return x;
}

BigFloat* bigfloat_create(const char* s) {
    if (!s) return bf_zero();
    if (!strcasecmp(s, "nan")) return bf_nan();
    if (!strcasecmp(s, "inf") || !strcasecmp(s, "+inf")) return bf_inf(false);
    if (!strcasecmp(s, "-inf")) return bf_inf(true);

    BigFloat* x = bf_new();
    const char* p = s;
    
    if (*p == '-' || *p == '+') {
        x->neg = (*p == '-');
        p++;
    }

    const char* dot = strchr(p, '.');
    int frac = dot ? strlen(dot + 1) : 0;

    free(x->digits);
    x->digits = calloc(strlen(p) + 1, 1);
    x->len = 0;

    for (; *p; p++) {
        if (*p == '.') continue;
        if (isdigit(*p)) {
            x->digits[x->len++] = *p;
        } else {
            // Некорректный символ - возвращаем NaN
            free(x->digits);
            x->digits = strdup("0");
            x->len = 1;
            x->is_nan = true;
            return x;
        }
    }
    
    if (!x->len) {
        x->digits[0] = '0';
        x->len = 1;
    }

    x->decimal_pos = frac;
    bf_trim(x);
    return x;
}

char* bigfloat_to_string(const BigFloat* x) {
    if (x->is_nan) return strdup("nan");
    if (x->is_inf) return strdup(x->neg ? "-inf" : "inf");

    int int_len = x->len - x->decimal_pos;
    if (int_len < 0) int_len = 0;

    // ВАЖНО: Если decimal_pos больше чем len, значит у нас ведущие нули в дробной части
    int leading_zeros_in_frac = 0;
    if (x->decimal_pos > x->len) {
        leading_zeros_in_frac = x->decimal_pos - x->len;
    }

    int size = int_len + x->decimal_pos + 3 + (int_len == 0 ? 1 : 0) + (x->neg ? 1 : 0);
    char* s = calloc(size, 1);
    int k = 0;

    if (x->neg) s[k++] = '-';

    if (int_len <= 0) {
        s[k++] = '0';
        s[k++] = '.';
        
        // Добавляем leading zeros
        for (int i = 0; i < leading_zeros_in_frac; i++) {
            s[k++] = '0';
        }
        
        // Добавляем оставшиеся zeros от -int_len
        for (int i = 0; i < -int_len - leading_zeros_in_frac; i++) {
            s[k++] = '0';
        }
        
        // Копируем цифры
        memcpy(s + k, x->digits, x->len);
        k += x->len;
    } else {
        // Целая часть
        memcpy(s + k, x->digits, int_len);
        k += int_len;
        
        if (x->decimal_pos > 0) {
            s[k++] = '.';
            
            // Если дробная часть короче чем decimal_pos, добавляем leading zeros
            int frac_digits_in_number = x->len - int_len;
            if (frac_digits_in_number < x->decimal_pos) {
                int missing_zeros = x->decimal_pos - frac_digits_in_number;
                for (int i = 0; i < missing_zeros; i++) {
                    s[k++] = '0';
                }
            }
            
            // Копируем дробные цифры
            int frac_to_copy = x->len - int_len;
            if (frac_to_copy > 0) {
                memcpy(s + k, x->digits + int_len, frac_to_copy);
                k += frac_to_copy;
            }
        }
    }

    s[k] = 0;
    return s;
}


/* integer helpers */

static int istrcmp(const char* a, int al, const char* b, int bl) {
    int a_start = 0;
    while (a_start < al - 1 && a[a_start] == '0') a_start++;
    int b_start = 0;
    while (b_start < bl - 1 && b[b_start] == '0') b_start++;
    
    int a_len = al - a_start;
    int b_len = bl - b_start;
    
    if (a_len != b_len) return a_len > b_len ? 1 : -1;
    
    for (int i = 0; i < a_len; i++) {
        if (a[a_start + i] != b[b_start + i]) {
            return a[a_start + i] > b[b_start + i] ? 1 : -1;
        }
    }
    return 0;
}

static char* iadd(const char* a, int al, const char* b, int bl, int* rl) {
    int n = (al > bl ? al : bl) + 1;
    char* r = malloc(n + 2);  // +2: для n цифр и нулевого символа
    memset(r, 0, n + 2);
    
    int c = 0;
    for (int i = 0; i < n; i++) {
        int da = al - 1 - i >= 0 ? a[al - 1 - i] - '0' : 0;
        int db = bl - 1 - i >= 0 ? b[bl - 1 - i] - '0' : 0;
        int s = da + db + c;
        r[n - 1 - i] = '0' + (s % 10);
        c = s / 10;
    }
    
    // Убираем ведущие нули
    int i = 0;
    while (i < n - 1 && r[i] == '0') {
        i++;
    }
    
    int new_len = n - i;
    if (i > 0) {
        memmove(r, r + i, new_len);
    }
    r[new_len] = '\0';
    
    // Освобождаем лишнюю память
    char* result = malloc(new_len + 1);
    memcpy(result, r, new_len + 1);
    free(r);
    
    *rl = new_len;
    return result;
}


static char* isub(const char* a, int al, const char* b, int bl, int* rl) {
    // a >= b должно соблюдаться! Если нет, возвращаем NULL
    int cmp = istrcmp(a, al, b, bl);
    if (cmp < 0) {
        *rl = 0;
        return NULL;
    }

    char* r = calloc(al + 1, 1);
    int br = 0;
    for (int i = 0; i < al; i++) {
        int da = a[al - 1 - i] - '0' - br;
        int db = (bl - 1 - i >= 0) ? (b[bl - 1 - i] - '0') : 0;
        if (da < db) {
            da += 10;
            br = 1;
        } else {
            br = 0;
        }
        r[al - 1 - i] = '0' + (da - db);
    }

    int i = 0;
    while (i < al - 1 && r[i] == '0') i++;
    memmove(r, r + i, al - i + 1);
    *rl = al - i;
    return r;
}





static char* imul(const char* a, int al, const char* b, int bl, int* rl) {
    int n = al + bl;
    int* temp = calloc(n, sizeof(int));

    for (int i = al - 1; i >= 0; i--) {
        int da = a[i] - '0';
        for (int j = bl - 1; j >= 0; j--) {
            int db = b[j] - '0';
            temp[i + j + 1] += da * db;
        }
    }

    // Перенос
    for (int i = n - 1; i > 0; i--) {
        temp[i - 1] += temp[i] / 10;
        temp[i] %= 10;
    }

    // Найдём первую значащую цифру
    int start = 0;
    while (start < n - 1 && temp[start] == 0) start++;

    *rl = n - start;
    char* r = calloc(*rl + 1, 1);
    for (int i = 0; i < *rl; i++) r[i] = temp[start + i] + '0';
    r[*rl] = 0;

    free(temp);
    return r;
}



static void bf_align(BigFloat* x, int dp) {
    if (x->decimal_pos < dp) {
        int add = dp - x->decimal_pos;
        x->digits = realloc(x->digits, x->len + add + 1);
        memset(x->digits + x->len, '0', add);
        x->len += add;
        x->digits[x->len] = 0;
        x->decimal_pos = dp;
    }
}

static int is_zero_string(const char* s, int len) {
    for (int i = 0; i < len; i++) {
        if (s[i] != '0') return 0;
    }
    return 1;
}

/* arithmetic */

BigFloat* bigfloat_add(const BigFloat* a, const BigFloat* b) {
    if (a->is_nan || b->is_nan) return bf_nan();
    
    if (a->is_inf || b->is_inf) {
        if (a->is_inf && b->is_inf) {
            if (a->neg == b->neg) return bf_inf(a->neg);
            return bf_nan(); // ∞ + (-∞) = NaN
        }
        return bf_inf(a->is_inf ? a->neg : b->neg);
    }

    BigFloat* x = bf_copy(a);
    BigFloat* y = bf_copy(b);
    int dp = x->decimal_pos > y->decimal_pos ? x->decimal_pos : y->decimal_pos;
    bf_align(x, dp);
    bf_align(y, dp);

    BigFloat* r = bf_new();
    free(r->digits);

    if (x->neg == y->neg) {
        r->digits = iadd(x->digits, x->len, y->digits, y->len, &r->len);
        r->neg = x->neg;
    } else {
        int cmp = istrcmp(x->digits, x->len, y->digits, y->len);
        if (cmp >= 0) {
            r->digits = isub(x->digits, x->len, y->digits, y->len, &r->len);
            r->neg = x->neg;
        } else {
            r->digits = isub(y->digits, y->len, x->digits, x->len, &r->len);
            r->neg = y->neg;
        }
    }

    r->decimal_pos = dp;
    bf_trim(r);
    bigfloat_free(x);
    bigfloat_free(y);
    return r;
}

BigFloat* bigfloat_sub(const BigFloat* a, const BigFloat* b) {
    BigFloat* nb = bf_copy(b);
    nb->neg = !nb->neg;
    BigFloat* r = bigfloat_add(a, nb);
    bigfloat_free(nb);
    return r;
}


BigFloat* bigfloat_mul(const BigFloat* a, const BigFloat* b) {
    if (a->is_nan || b->is_nan) return bf_nan();

    // ∞ * 0 = NaN
    if ((a->is_inf && is_zero_string(b->digits, b->len)) ||
        (b->is_inf && is_zero_string(a->digits, a->len))) {
        return bf_nan();
    }

    // ∞ * finite = ∞
    if (a->is_inf || b->is_inf) {
        bool result_neg = a->neg != b->neg;
        return bf_inf(result_neg);
    }

    // Полное умножение строк
    int rlen;
    char* rdigits = imul(a->digits, a->len, b->digits, b->len, &rlen);

    BigFloat* r = bf_new();
    free(r->digits);
    r->digits = rdigits;
    r->len = rlen;

    // decimal_pos = сумма decimal_pos исходных чисел
    r->decimal_pos = a->decimal_pos + b->decimal_pos;

    // Если дробная часть длиннее результата, добавляем ведущие нули
    if (r->decimal_pos > r->len) {
        int add = r->decimal_pos - r->len;
        r->digits = realloc(r->digits, r->len + add + 1);
        memmove(r->digits + add, r->digits, r->len);
        memset(r->digits, '0', add);
        r->len += add;
        r->digits[r->len] = 0;
    }

    r->neg = a->neg != b->neg;

    bf_trim(r);
    bf_limit_precision(r, BF_PRECISION);
    return r;
}


BigFloat* bigfloat_div(const BigFloat* a, const BigFloat* b) {
    if (a->is_nan || b->is_nan) return bf_nan();

    if (b->is_inf) {
        BigFloat* zero = bf_zero();
        zero->neg = a->neg != b->neg;
        return zero;
    }
    if (a->is_inf) return bf_inf(a->neg != b->neg);
    if (is_zero_string(b->digits, b->len)) {
        if (is_zero_string(a->digits, a->len)) return bf_nan();
        return bf_inf(a->neg != b->neg);
    }

    bool result_neg = a->neg != b->neg;

    BigFloat* num = bf_copy(a);
    BigFloat* den = bf_copy(b);
    num->neg = false;
    den->neg = false;

    int max_decimal = num->decimal_pos > den->decimal_pos ? num->decimal_pos : den->decimal_pos;

    if (num->decimal_pos < max_decimal) {
        int add_zeros = max_decimal - num->decimal_pos;
        num->digits = realloc(num->digits, num->len + add_zeros + 1);
        memset(num->digits + num->len, '0', add_zeros);
        num->len += add_zeros;
        num->digits[num->len] = '\0';
        num->decimal_pos = max_decimal;
    }
    
    if (den->decimal_pos < max_decimal) {
        int add_zeros = max_decimal - den->decimal_pos;
        den->digits = realloc(den->digits, den->len + add_zeros + 1);
        memset(den->digits + den->len, '0', add_zeros);
        den->len += add_zeros;
        den->digits[den->len] = '\0';
        den->decimal_pos = max_decimal;
    }

    int extra_zeros = BF_PRECISION;
    if (extra_zeros > 0) {
        num->digits = realloc(num->digits, num->len + extra_zeros + 1);
        memset(num->digits + num->len, '0', extra_zeros);
        num->len += extra_zeros;
        num->digits[num->len] = '\0';
        num->decimal_pos += extra_zeros;
    }

    int nlen = num->len;
    int dlen = den->len;

    while (istrcmp(num->digits, nlen, den->digits, dlen) < 0) {
        num->digits = realloc(num->digits, nlen + 2);
        num->digits[nlen] = '0';
        num->digits[nlen + 1] = '\0';
        num->len++;
        nlen++;
        num->decimal_pos++;
    }

    char* rdigits = calloc(nlen + 1, 1);
    char* rem = malloc(nlen + 2);
    int rem_len = 0;


for (int i = 0; i < nlen; i++) {
    rem[rem_len++] = num->digits[i];
    rem[rem_len] = '\0';

    int q = 0;

    // Убираем ведущие нули
    int start = 0;
    while (start < rem_len && rem[start] == '0') start++;
    int effective_rem_len = rem_len - start;

    if (effective_rem_len > 0) {
        while (effective_rem_len >= dlen && istrcmp(rem + start, effective_rem_len, den->digits, dlen) >= 0) {
            if (is_zero_string(rem + start, effective_rem_len)) break;

            int tmp_len;
            char* tmp = isub(rem + start, effective_rem_len, den->digits, dlen, &tmp_len);

            // Сдвигаем tmp в начало rem + start
            if (tmp_len > 0) {
                memmove(rem + start, tmp, tmp_len);
            }
            rem_len = start + tmp_len;
            rem[rem_len] = '\0';
            free(tmp);

            q++;
            start = 0;
            while (start < rem_len && rem[start] == '0') start++;
            effective_rem_len = rem_len - start;
        }
    }

    rdigits[i] = '0' + q;

    // Убираем ведущие нули из rem для следующей итерации
    if (start > 0) {
        memmove(rem, rem + start, rem_len - start);
        rem_len -= start;
        rem[rem_len] = '\0';
    }
}

    BigFloat* r = bf_new();
    free(r->digits);
    r->digits = rdigits;
    r->len = nlen;

    r->decimal_pos = num->decimal_pos - den->decimal_pos;

    if (r->decimal_pos < 0) {
        int add_zeros = -r->decimal_pos;
        r->digits = realloc(r->digits, r->len + add_zeros + 1);
        memmove(r->digits + add_zeros, r->digits, r->len);
        memset(r->digits, '0', add_zeros);
        r->len += add_zeros;
        r->digits[r->len] = '\0';
        r->decimal_pos = 0;
    }

    r->neg = result_neg;
    bf_trim(r);
    bf_limit_precision(r, BF_PRECISION);  // ← ДОБАВЬ ЭТУ СТРОКУ
    free(rem);
    bigfloat_free(num);
    bigfloat_free(den);
    return r;
}


BigFloat* bigfloat_mod(const BigFloat* a, const BigFloat* b) {
    if (a->is_nan || b->is_nan || a->is_inf || b->is_inf) return bf_nan();
    if (is_zero_string(b->digits, b->len)) return bf_nan(); // mod by zero
    
    BigFloat* d = bigfloat_div(a, b);
    
    // Округляем частное до целого вниз
    BigFloat* d_int = bf_copy(d);
    if (d_int->decimal_pos > 0) {
        d_int->len -= d_int->decimal_pos;
        d_int->decimal_pos = 0;
        bf_trim(d_int);
    }
    
    BigFloat* m = bigfloat_mul(d_int, b);
    BigFloat* r = bigfloat_sub(a, m);
    
    bigfloat_free(d);
    bigfloat_free(d_int);
    bigfloat_free(m);
    
    return r;
}

int bigfloat_cmp(const BigFloat* a, const BigFloat* b) {
    if (a->is_nan || b->is_nan) return 0;
    
    if (a->is_inf && b->is_inf) {
        if (a->neg && !b->neg) return -1;
        if (!a->neg && b->neg) return 1;
        return 0;
    }
    
    if (a->is_inf) return a->neg ? -1 : 1;
    if (b->is_inf) return b->neg ? 1 : -1;
    
    if (a->neg != b->neg) return a->neg ? -1 : 1;
    
    // Вычисляем разность
    BigFloat* diff = bigfloat_sub(a, b);
    
    // Эпсилон для сравнения
    BigFloat* epsilon = bigfloat_create("0.000000000000001");
    
    int result;
    if (bigfloat_cmp_abs(diff, epsilon) <= 0) {
        // |diff| <= epsilon => считаем равными
        result = 0;
    } else if (diff->neg) {
        result = -1;
    } else {
        result = 1;
    }
    
    bigfloat_free(diff);
    bigfloat_free(epsilon);
    
    return a->neg ? -result : result;
}

// Вспомогательная функция для сравнения модулей
static int bigfloat_cmp_abs(const BigFloat* a, const BigFloat* b) {
    BigFloat* a_abs = a->neg ? bigfloat_neg(a) : bf_copy(a);
    BigFloat* b_abs = b->neg ? bigfloat_neg(b) : bf_copy(b);
    
    // Простое сравнение без учета знака
    BigFloat* x = bf_copy(a_abs);
    BigFloat* y = bf_copy(b_abs);
    int dp = x->decimal_pos > y->decimal_pos ? x->decimal_pos : y->decimal_pos;
    bf_align(x, dp);
    bf_align(y, dp);
    
    int cmp = istrcmp(x->digits, x->len, y->digits, y->len);
    
    bigfloat_free(a_abs);
    bigfloat_free(b_abs);
    bigfloat_free(x);
    bigfloat_free(y);
    
    return cmp;
}


BigFloat* bigfloat_sqrt(const BigFloat* a) {
    if (a->is_nan || a->neg) return bf_nan();
    if (a->is_inf) return bf_inf(false);
    if (is_zero_string(a->digits, a->len)) return bf_zero();

    BigFloat* x = bf_copy(a);
    BigFloat* half = bigfloat_create("0.5");

    for (int i = 0; i < BF_SQRT_ITERS; i++) {
        BigFloat* a_div_x = bigfloat_div(a, x);
        BigFloat* sum = bigfloat_add(x, a_div_x);
        BigFloat* new_x = bigfloat_mul(sum, half);

        bigfloat_free(x);
        bigfloat_free(a_div_x);
        bigfloat_free(sum);

        x = new_x;
    }

    bigfloat_free(half);
    return x;
}


BigFloat* bigfloat_neg(const BigFloat* a) {
    BigFloat* r = bf_copy(a);
    if (!r->is_nan && !r->is_inf) {
        r->neg = !a->neg;
    }
    return r;
}

bool bigfloat_eq(const BigFloat* a, const BigFloat* b) {
    return bigfloat_cmp(a, b) == 0;
}

bool bigfloat_lt(const BigFloat* a, const BigFloat* b) {
    return bigfloat_cmp(a, b) < 0;
}

bool bigfloat_le(const BigFloat* a, const BigFloat* b) {
    return bigfloat_cmp(a, b) <= 0;
}

bool bigfloat_gt(const BigFloat* a, const BigFloat* b) {
    return bigfloat_cmp(a, b) > 0;
}

bool bigfloat_ge(const BigFloat* a, const BigFloat* b) {
    return bigfloat_cmp(a, b) >= 0;
}

BigFloat* bigfloat_zero(void) { return bf_zero(); }
BigFloat* bigfloat_one(void) { return bf_one(); }
BigFloat* bigfloat_nan(void) { return bf_nan(); }
BigFloat* bigfloat_inf(bool negative) { return bf_inf(negative); }
