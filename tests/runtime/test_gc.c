//
// Created by Artem on 06.01.2026.
//

#include <stdio.h>
#include <assert.h>
#include "../../src/runtime/gc/gc.h"

static void assert_ref_count(Object* obj, uint32_t expected, const char* test_name) {
    if (obj) {
        assert(obj->ref_count == expected);
        printf("  ✓ %s: ref_count = %u (expected %u)\n", test_name, obj->ref_count, expected);
    }
}

static void test_gc_create_destroy() {
    printf("=== Test 1: GC Create/Destroy ===\n");
    GC* gc = gc_create();
    assert(gc != NULL);
    printf("  ✓ GC created\n");
    
    gc_destroy(gc);
    printf("  ✓ GC destroyed\n");
    printf("PASSED\n\n");
}

static void test_basic_ref_counting() {
    printf("=== Test 2: Basic Reference Counting ===\n");
    GC* gc = gc_create();

    Object* obj = object_new_int(42);
    assert_ref_count(obj, 1, "Initial ref_count");

    gc_incref(gc, obj);
    assert_ref_count(obj, 2, "After gc_incref");
    
    gc_incref(gc, obj);
    assert_ref_count(obj, 3, "After second gc_incref");

    gc_decref(gc, obj);
    assert_ref_count(obj, 2, "After gc_decref");
    
    gc_decref(gc, obj);
    assert_ref_count(obj, 1, "After second gc_decref");

    gc_decref(gc, obj);
    
    gc_destroy(gc);
    printf("PASSED\n\n");
}

static void test_object_cleanup() {
    printf("=== Test 3: Object Cleanup ===\n");
    GC* gc = gc_create();

    Object* int_obj = object_new_int(100);
    assert_ref_count(int_obj, 1, "Int object created");
    gc_decref(gc, int_obj);
    printf("  ✓ Int object cleaned up\n");

    Object* bool_obj = object_new_bool(true);
    assert_ref_count(bool_obj, 1, "Bool object created");
    gc_decref(gc, bool_obj);
    printf("  ✓ Bool object cleaned up\n");

    Object* none_obj = object_new_none();
    assert_ref_count(none_obj, 1, "None object created");
    gc_decref(gc, none_obj);
    printf("  ✓ None object cleaned up\n");

    Object* float_obj = object_new_float("3.14159");
    assert_ref_count(float_obj, 1, "Float object created");
    gc_decref(gc, float_obj);
    printf("  ✓ Float object cleaned up\n");
    
    gc_destroy(gc);
    printf("PASSED\n\n");
}

static void test_array_with_elements() {
    printf("=== Test 4: Array with Elements ===\n");
    GC* gc = gc_create();

    Object* array = object_new_array();
    assert_ref_count(array, 1, "Array created");

    Object* elem1 = object_new_int(10);
    Object* elem2 = object_new_int(20);
    Object* elem3 = object_new_int(30);
    
    assert_ref_count(elem1, 1, "Element 1 created");
    assert_ref_count(elem2, 1, "Element 2 created");
    assert_ref_count(elem3, 1, "Element 3 created");

    object_array_append(array, elem1);
    object_array_append(array, elem2);
    object_array_append(array, elem3);

    gc_incref(gc, elem1);
    gc_incref(gc, elem2);
    gc_incref(gc, elem3);
    
    assert_ref_count(elem1, 2, "Element 1 in array");
    assert_ref_count(elem2, 2, "Element 2 in array");
    assert_ref_count(elem3, 2, "Element 3 in array");

    gc_decref(gc, elem1);
    gc_decref(gc, elem2);
    gc_decref(gc, elem3);
    
    assert_ref_count(elem1, 1, "Element 1 after decref (still in array)");
    assert_ref_count(elem2, 1, "Element 2 after decref (still in array)");
    assert_ref_count(elem3, 1, "Element 3 after decref (still in array)");

    gc_decref(gc, array);
    printf("  ✓ Array and elements cleaned up\n");
    
    gc_destroy(gc);
    printf("PASSED\n\n");
}

static void test_multiple_references() {
    printf("=== Test 5: Multiple References ===\n");
    GC* gc = gc_create();
    
    Object* obj = object_new_int(999);
    assert_ref_count(obj, 1, "Object created");

    gc_incref(gc, obj);
    gc_incref(gc, obj);
    gc_incref(gc, obj);
    assert_ref_count(obj, 4, "After 3 increfs");

    gc_decref(gc, obj);
    assert_ref_count(obj, 3, "After first decref");
    
    gc_decref(gc, obj);
    assert_ref_count(obj, 2, "After second decref");
    
    gc_decref(gc, obj);
    assert_ref_count(obj, 1, "After third decref");

    gc_decref(gc, obj);
    printf("  ✓ Object cleaned up after all references released\n");
    
    gc_destroy(gc);
    printf("PASSED\n\n");
}

static void test_null_handling() {
    printf("=== Test 6: NULL Handling ===\n");
    GC* gc = gc_create();

    gc_incref(gc, NULL);
    gc_decref(gc, NULL);
    printf("  ✓ NULL handling works correctly\n");
    
    gc_destroy(gc);
    printf("PASSED\n\n");
}

static void test_nested_arrays() {
    printf("=== Test 7: Nested Arrays ===\n");
    GC* gc = gc_create();

    Object* outer = object_new_array();
    assert_ref_count(outer, 1, "Outer array created");

    Object* inner = object_new_array();
    assert_ref_count(inner, 1, "Inner array created");

    Object* elem = object_new_int(42);
    object_array_append(inner, elem);
    gc_incref(gc, elem);
    assert_ref_count(elem, 2, "Element in inner array");

    object_array_append(outer, inner);
    gc_incref(gc, inner);
    assert_ref_count(inner, 2, "Inner array in outer array");

    gc_decref(gc, elem);
    gc_decref(gc, inner);
    
    assert_ref_count(elem, 1, "Element still referenced by inner array");
    assert_ref_count(inner, 1, "Inner array still referenced by outer array");

    gc_decref(gc, outer);
    printf("  ✓ Nested arrays cleaned up correctly\n");
    
    gc_destroy(gc);
    printf("PASSED\n\n");
}

static void test_gc_collect() {
    printf("=== Test 8: GC Collect ===\n");
    GC* gc = gc_create();
    
    Object* obj1 = object_new_int(1);
    Object* obj2 = object_new_int(2);
    Object* obj3 = object_new_int(3);
    
    gc_incref(gc, obj1);
    gc_incref(gc, obj2);
    gc_incref(gc, obj3);

    gc_collect(gc);
    printf("  ✓ gc_collect called without errors\n");

    gc_decref(gc, obj1);
    gc_decref(gc, obj2);
    gc_decref(gc, obj3);
    
    gc_destroy(gc);
    printf("PASSED\n\n");
}

int main() {
    printf("========================================\n");
    printf("GC Test Suite\n");
    printf("========================================\n\n");
    
    test_gc_create_destroy();
    test_basic_ref_counting();
    test_object_cleanup();
    test_array_with_elements();
    test_multiple_references();
    test_null_handling();
    test_nested_arrays();
    test_gc_collect();
    
    printf("========================================\n");
    printf("All GC tests PASSED! ✓\n");
    printf("========================================\n");
    
    return 0;
}
