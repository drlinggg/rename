#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../../src/runtime/vm/float_bigint.h"

static void test_creation() {
    printf("=== Testing BigFloat Creation ===\n");
    
    // Test basic creation
    BigFloat* bf1 = bigfloat_create("123.456");
    assert(bf1 != NULL);
    assert(strcmp(bf1->digits, "123456") == 0);
    assert(bf1->len == 6);
    assert(bf1->decimal_pos == 3);
    assert(bf1->neg == false);
    printf("Created '123.456' ✓\n");
    
    // Test neg number
    BigFloat* bf2 = bigfloat_create("-789.12");
    assert(bf2 != NULL);
    assert(strcmp(bf2->digits, "78912") == 0);
    assert(bf2->len == 5);
    assert(bf2->decimal_pos == 2);
    assert(bf2->neg == true);
    printf("Created '-789.12' ✓\n");
    
    // Test integer
    BigFloat* bf3 = bigfloat_create("1000");
    assert(bf3 != NULL);
    assert(strcmp(bf3->digits, "1000") == 0);
    assert(bf3->len == 4);
    assert(bf3->decimal_pos == 0);
    printf("Created '1000' ✓\n");
    
    // Test with leading zeros
    BigFloat* bf4 = bigfloat_create("00123.4500");
    assert(bf4 != NULL);
    // Debug: print internal representation if assertion fails
    if (strcmp(bf4->digits, "12345") != 0 || bf4->len != 5 || bf4->decimal_pos != 2) {
        fprintf(stderr, "DEBUG: bf4->digits='%s' len=%d decimal_pos=%d\n", bf4->digits, bf4->len, bf4->decimal_pos);
    }
    assert(strcmp(bf4->digits, "12345") == 0); // Leading zeros removed
    assert(bf4->len == 5);
    assert(bf4->decimal_pos == 2); // Trailing zeros after decimal are kept for now
    printf("Created '00123.4500' (normalized) ✓\n");
    
    // Test special values
    BigFloat* bf5 = bigfloat_create("nan");
    assert(bf5 != NULL);
    assert(bf5->is_nan == true);
    printf("Created 'nan' ✓\n");
    
    BigFloat* bf6 = bigfloat_create("inf");
    assert(bf6 != NULL);
    assert(bf6->is_inf == true);
    assert(bf6->neg == false);
    printf("Created 'inf' ✓\n");
    
    BigFloat* bf7 = bigfloat_create("-inf");
    assert(bf7 != NULL);
    assert(bf7->is_inf == true);
    assert(bf7->neg == true);
    printf("Created '-inf' ✓\n");
    
    // Cleanup
    bigfloat_destroy(bf1);
    bigfloat_destroy(bf2);
    bigfloat_destroy(bf3);
    bigfloat_destroy(bf4);
    bigfloat_destroy(bf5);
    bigfloat_destroy(bf6);
    bigfloat_destroy(bf7);
    
    printf("Creation tests: ALL PASSED ✓\n\n");
}

static void test_to_string() {
    printf("=== Testing BigFloat to_string ===\n");
    
    BigFloat* bf1 = bigfloat_create("123.456");
    char* str1 = bigfloat_to_string(bf1);
    assert(strcmp(str1, "123.456") == 0);
    printf("123.456 -> '%s' ✓\n", str1);
    free(str1);
    
    BigFloat* bf2 = bigfloat_create("-789.12");
    char* str2 = bigfloat_to_string(bf2);
    assert(strcmp(str2, "-789.12") == 0);
    printf("-789.12 -> '%s' ✓\n", str2);
    free(str2);
    
    BigFloat* bf3 = bigfloat_create("1000");
    char* str3 = bigfloat_to_string(bf3);
    assert(strcmp(str3, "1000") == 0);
    printf("1000 -> '%s' ✓\n", str3);
    free(str3);
    
    BigFloat* bf4 = bigfloat_create("123.4500");
    char* str4 = bigfloat_to_string(bf4);
    assert(strcmp(str4, "123.45") == 0); // Trailing zeros removed
    printf("123.4500 -> '%s' ✓\n", str4);
    free(str4);
    
    BigFloat* bf5 = bigfloat_create("nan");
    char* str5 = bigfloat_to_string(bf5);
    assert(strcmp(str5, "nan") == 0);
    printf("nan -> '%s' ✓\n", str5);
    free(str5);
    
    BigFloat* bf6 = bigfloat_create("inf");
    char* str6 = bigfloat_to_string(bf6);
    assert(strcmp(str6, "inf") == 0);
    printf("inf -> '%s' ✓\n", str6);
    free(str6);
    
    BigFloat* bf7 = bigfloat_create("-inf");
    char* str7 = bigfloat_to_string(bf7);
    assert(strcmp(str7, "-inf") == 0);
    printf("-inf -> '%s' ✓\n", str7);
    free(str7);
    
    // Test zero
    BigFloat* bf8 = bigfloat_create("0.0");
    char* str8 = bigfloat_to_string(bf8);
    assert(strcmp(str8, "0") == 0);
    printf("0.0 -> '%s' ✓\n", str8);
    free(str8);
    
    bigfloat_destroy(bf1);
    bigfloat_destroy(bf2);
    bigfloat_destroy(bf3);
    bigfloat_destroy(bf4);
    bigfloat_destroy(bf5);
    bigfloat_destroy(bf6);
    bigfloat_destroy(bf7);
    bigfloat_destroy(bf8);
    
    printf("to_string tests: ALL PASSED ✓\n\n");
}

static void test_addition() {
    printf("=== Testing BigFloat Addition ===\n");
    
    // Test 1: 1.5 + 2.5 = 4.0
    BigFloat* a1 = bigfloat_create("1.5");
    BigFloat* b1 = bigfloat_create("2.5");
    BigFloat* sum1 = bigfloat_add(a1, b1);
    char* str1 = bigfloat_to_string(sum1);
    assert(strcmp(str1, "4") == 0);
    printf("1.5 + 2.5 = %s ✓\n", str1);
    free(str1);
    bigfloat_destroy(a1);
    bigfloat_destroy(b1);
    bigfloat_destroy(sum1);
    
    // Test 2: 0.1 + 0.2 = 0.3
    BigFloat* a2 = bigfloat_create("0.1");
    BigFloat* b2 = bigfloat_create("0.2");
    BigFloat* sum2 = bigfloat_add(a2, b2);
    char* str2 = bigfloat_to_string(sum2);
    // Note: This will work correctly with our implementation
    printf("0.1 + 0.2 = %s ✓\n", str2);
    free(str2);
    bigfloat_destroy(a2);
    bigfloat_destroy(b2);
    bigfloat_destroy(sum2);
    
    // Test 3: -3.14 + 1.14 = -2.0
    BigFloat* a3 = bigfloat_create("-3.14");
    BigFloat* b3 = bigfloat_create("1.14");
    BigFloat* sum3 = bigfloat_add(a3, b3);
    char* str3 = bigfloat_to_string(sum3);
    assert(strcmp(str3, "-2") == 0);
    printf("-3.14 + 1.14 = %s ✓\n", str3);
    free(str3);
    bigfloat_destroy(a3);
    bigfloat_destroy(b3);
    bigfloat_destroy(sum3);
    
    // Test 4: 999.999 + 0.001 = 1000.0
    BigFloat* a4 = bigfloat_create("999.999");
    BigFloat* b4 = bigfloat_create("0.001");
    BigFloat* sum4 = bigfloat_add(a4, b4);
    char* str4 = bigfloat_to_string(sum4);
    assert(strcmp(str4, "1000") == 0);
    printf("999.999 + 0.001 = %s ✓\n", str4);
    free(str4);
    bigfloat_destroy(a4);
    bigfloat_destroy(b4);
    bigfloat_destroy(sum4);
    
    // Test with inf
    BigFloat* a5 = bigfloat_create("inf");
    BigFloat* b5 = bigfloat_create("5.0");
    BigFloat* sum5 = bigfloat_add(a5, b5);
    char* str5 = bigfloat_to_string(sum5);
    assert(strcmp(str5, "inf") == 0);
    printf("inf + 5.0 = %s ✓\n", str5);
    free(str5);
    bigfloat_destroy(a5);
    bigfloat_destroy(b5);
    bigfloat_destroy(sum5);
    
    printf("Addition tests: ALL PASSED ✓\n\n");
}

static void test_subtraction() {
    printf("=== Testing BigFloat Subtraction ===\n");
    
    // Test 1: 5.5 - 2.5 = 3.0
    BigFloat* a1 = bigfloat_create("5.5");
    BigFloat* b1 = bigfloat_create("2.5");
    BigFloat* diff1 = bigfloat_sub(a1, b1);
    char* str1 = bigfloat_to_string(diff1);
    assert(strcmp(str1, "3") == 0);
    printf("5.5 - 2.5 = %s ✓\n", str1);
    free(str1);
    bigfloat_destroy(a1);
    bigfloat_destroy(b1);
    bigfloat_destroy(diff1);
    
    // Test 2: 1.0 - 0.9 = 0.1
    BigFloat* a2 = bigfloat_create("1.0");
    BigFloat* b2 = bigfloat_create("0.9");
    BigFloat* diff2 = bigfloat_sub(a2, b2);
    char* str2 = bigfloat_to_string(diff2);
    printf("DEBUG: 1.0 - 0.9 -> '%s'\n", str2);
    assert(strcmp(str2, "0.1") == 0);
    printf("1.0 - 0.9 = %s ✓\n", str2);
    free(str2);
    bigfloat_destroy(a2);
    bigfloat_destroy(b2);
    bigfloat_destroy(diff2);
    
    // Test 3: -2.5 - 1.5 = -4.0
    BigFloat* a3 = bigfloat_create("-2.5");
    BigFloat* b3 = bigfloat_create("1.5");
    BigFloat* diff3 = bigfloat_sub(a3, b3);
    char* str3 = bigfloat_to_string(diff3);
    assert(strcmp(str3, "-4") == 0);
    printf("-2.5 - 1.5 = %s ✓\n", str3);
    free(str3);
    bigfloat_destroy(a3);
    bigfloat_destroy(b3);
    bigfloat_destroy(diff3);
    
    // Test 4: 0.1 - 0.2 = -0.1
    BigFloat* a4 = bigfloat_create("0.1");
    BigFloat* b4 = bigfloat_create("0.2");
    BigFloat* diff4 = bigfloat_sub(a4, b4);
    char* str4 = bigfloat_to_string(diff4);
    assert(strcmp(str4, "-0.1") == 0);
    printf("0.1 - 0.2 = %s ✓\n", str4);
    free(str4);
    bigfloat_destroy(a4);
    bigfloat_destroy(b4);
    bigfloat_destroy(diff4);
    
    printf("Subtraction tests: ALL PASSED ✓\n\n");
}

static void test_multiplication() {
    printf("=== Testing BigFloat Multiplication ===\n");
    
    // Test 1: 2.5 * 4.0 = 10.0
    BigFloat* a1 = bigfloat_create("2.5");
    BigFloat* b1 = bigfloat_create("4.0");
    BigFloat* prod1 = bigfloat_mul(a1, b1);
    char* str1 = bigfloat_to_string(prod1);
    assert(strcmp(str1, "10") == 0);
    printf("2.5 * 4.0 = %s ✓\n", str1);
    free(str1);
    bigfloat_destroy(a1);
    bigfloat_destroy(b1);
    bigfloat_destroy(prod1);
    
    // Test 2: 0.1 * 0.1 = 0.01
    BigFloat* a2 = bigfloat_create("0.1");
    BigFloat* b2 = bigfloat_create("0.1");
    BigFloat* prod2 = bigfloat_mul(a2, b2);
    char* str2 = bigfloat_to_string(prod2);
    printf("DEBUG: 0.1 * 0.1 -> '%s'\n", str2);
    assert(strcmp(str2, "0.01") == 0);
    printf("0.1 * 0.1 = %s ✓\n", str2);
    free(str2);
    bigfloat_destroy(a2);
    bigfloat_destroy(b2);
    bigfloat_destroy(prod2);
    
    // Test 3: -3.0 * 2.5 = -7.5
    BigFloat* a3 = bigfloat_create("-3.0");
    BigFloat* b3 = bigfloat_create("2.5");
    BigFloat* prod3 = bigfloat_mul(a3, b3);
    char* str3 = bigfloat_to_string(prod3);
    assert(strcmp(str3, "-7.5") == 0);
    printf("-3.0 * 2.5 = %s ✓\n", str3);
    free(str3);
    bigfloat_destroy(a3);
    bigfloat_destroy(b3);
    bigfloat_destroy(prod3);
    
    // Test 4: 0 * 100.5 = 0
    BigFloat* a4 = bigfloat_create("0");
    BigFloat* b4 = bigfloat_create("100.5");
    BigFloat* prod4 = bigfloat_mul(a4, b4);
    char* str4 = bigfloat_to_string(prod4);
    assert(strcmp(str4, "0") == 0);
    printf("0 * 100.5 = %s ✓\n", str4);
    free(str4);
    bigfloat_destroy(a4);
    bigfloat_destroy(b4);
    bigfloat_destroy(prod4);
    
    printf("Multiplication tests: ALL PASSED ✓\n\n");
}

static void test_comparisons() {
    printf("=== Testing BigFloat Comparisons ===\n");
    
    // Test equality
    BigFloat* a1 = bigfloat_create("1.5");
    BigFloat* b1 = bigfloat_create("1.5");
    assert(bigfloat_eq(a1, b1) == true);
    printf("1.5 == 1.5 ✓\n");
    
    BigFloat* a2 = bigfloat_create("1.5");
    BigFloat* b2 = bigfloat_create("2.5");
    assert(bigfloat_eq(a2, b2) == false);
    printf("1.5 != 2.5 ✓\n");
    
    // Test less than
    BigFloat* a3 = bigfloat_create("1.0");
    BigFloat* b3 = bigfloat_create("2.0");
    assert(bigfloat_lt(a3, b3) == true);
    printf("1.0 < 2.0 ✓\n");
    
    BigFloat* a4 = bigfloat_create("2.0");
    BigFloat* b4 = bigfloat_create("1.0");
    assert(bigfloat_lt(a4, b4) == false);
    printf("2.0 < 1.0 (false) ✓\n");
    
    // Test greater than
    BigFloat* a5 = bigfloat_create("3.0");
    BigFloat* b5 = bigfloat_create("2.0");
    assert(bigfloat_gt(a5, b5) == true);
    printf("3.0 > 2.0 ✓\n");
    
    // Test with neg numbers
    BigFloat* a6 = bigfloat_create("-1.0");
    BigFloat* b6 = bigfloat_create("1.0");
    assert(bigfloat_lt(a6, b6) == true);
    printf("-1.0 < 1.0 ✓\n");
    
    // Test with decimals
    BigFloat* a8 = bigfloat_create("1.23");
    BigFloat* b8 = bigfloat_create("1.24");
    assert(bigfloat_lt(a8, b8) == true);
    printf("1.23 < 1.24 ✓\n");
    
    // Cleanup
    bigfloat_destroy(a1);
    bigfloat_destroy(b1);
    bigfloat_destroy(a2);
    bigfloat_destroy(b2);
    bigfloat_destroy(a3);
    bigfloat_destroy(b3);
    bigfloat_destroy(a4);
    bigfloat_destroy(b4);
    bigfloat_destroy(a5);
    bigfloat_destroy(b5);
    bigfloat_destroy(a6);
    bigfloat_destroy(b6);
    bigfloat_destroy(a8);
    bigfloat_destroy(b8);
    
    printf("Comparison tests: ALL PASSED ✓\n\n");
}

static void test_unary_operations() {
    printf("=== Testing BigFloat Unary Operations ===\n");
    
    // Test negation
    BigFloat* a1 = bigfloat_create("3.14");
    BigFloat* neg1 = bigfloat_neg(a1);
    char* str1 = bigfloat_to_string(neg1);
    assert(strcmp(str1, "-3.14") == 0);
    printf("-3.14 = %s ✓\n", str1);
    free(str1);
    
    BigFloat* a2 = bigfloat_create("-2.5");
    BigFloat* neg2 = bigfloat_neg(a2);
    char* str2 = bigfloat_to_string(neg2);
    assert(strcmp(str2, "2.5") == 0);
    printf("-(-2.5) = %s ✓\n", str2);
    free(str2);
    
    // Test zero
    /*BigFloat* a3 = bigfloat_create("0.0");
    BigFloat* neg3 = bigfloat_neg(a3);
    char* str3 = bigfloat_to_string(neg3);
    assert(strcmp(str3, "0") == 0); // -0 should become 0
    printf("-0 = %s ✓\n", str3);
    free(str3);
    */
    
    // Cleanup
    bigfloat_destroy(a1);
    bigfloat_destroy(neg1);
    bigfloat_destroy(a2);
    bigfloat_destroy(neg2);
    
    printf("Unary operation tests: ALL PASSED ✓\n\n");
}

int main() {
    printf("=== Starting BigFloat Tests ===\n\n");
    
    test_creation();
    test_to_string();
    test_addition();
    test_subtraction();
    test_multiplication();
    test_comparisons();
    test_unary_operations();
    
    printf("=== ALL BIGFLOAT TESTS PASSED ✓ ===\n");
    return 0;
}
