#include "ej1.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>

#define MAX_RESULT_LEN 1048576 // Maximum length allowed for concatenated string result

// Conditional compilation to switch between C and ASM implementations
#if USE_ASM_IMPL
#define string_proc_list_create_impl string_proc_list_create_asm
#define string_proc_node_create_impl string_proc_node_create_asm
#define string_proc_list_add_node_impl string_proc_list_add_node_asm
#define string_proc_list_concat_impl string_proc_list_concat_asm
#else
#define string_proc_list_create_impl string_proc_list_create
#define string_proc_node_create_impl string_proc_node_create
#define string_proc_list_add_node_impl string_proc_list_add_node
#define string_proc_list_concat_impl string_proc_list_concat
#endif


void test_create_destroy_list(){
    printf("Running test: %s\n", __func__);
    string_proc_list * list = string_proc_list_create_impl();
    assert(list != NULL);
    assert(list->first == NULL);
    assert(list->last == NULL);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}


void test_create_destroy_node(){
    printf("Running test: %s\n", __func__);
    char* hash_val = "hash";
    string_proc_node* node  = string_proc_node_create_impl(0, hash_val);
    assert(node != NULL);
    assert(node->type == 0);
    assert(node->hash == hash_val); // Check if hash pointer is correctly assigned
    assert(node->next == NULL);
    assert(node->previous == NULL);
    string_proc_node_destroy(node);
    printf("Finished test: %s\n", __func__);
}


void test_create_list_add_nodes()
{
    printf("Running test: %s\n", __func__);
    string_proc_list * list = string_proc_list_create_impl();
    assert(list != NULL);
    string_proc_list_add_node_impl(list, 0, "hola");
    string_proc_list_add_node_impl(list, 1, "a");
    string_proc_list_add_node_impl(list, 0, "todos!");
    assert(list->first != NULL);
    assert(list->last != NULL);
    // Verify the order and content of first and last nodes after additions
    assert(strcmp(list->first->hash, "hola") == 0);
    assert(strcmp(list->last->hash, "todos!") == 0);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}


void test_list_concat_basic()
{
    printf("Running test: %s\n", __func__);
    string_proc_list * list = string_proc_list_create_impl();
    assert(list != NULL);
    string_proc_list_add_node_impl(list, 0, "hola");
    string_proc_list_add_node_impl(list, 1, "a"); // Node with different type, should be skipped
    string_proc_list_add_node_impl(list, 0, "todos!");
    char* initial_hash = "hash:";
    char* new_hash = string_proc_list_concat_impl(list, 0, initial_hash); // Concatenate nodes of type 0
    assert(new_hash != NULL);
    assert(strcmp(new_hash, "hash:holatodos!") == 0); // Verify concatenated result
    string_proc_list_destroy(list);
    free(new_hash); // Free the memory allocated by concat
    printf("Finished test: %s\n", __func__);
}

// Tests concatenation on an empty list.
void test_empty_list_concat() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);
    char* initial_hash = "empty_test:";
    char* result_hash = string_proc_list_concat_impl(list, 0, initial_hash);

    assert(result_hash != NULL);
    // Ensure result is a new copy, not the original string
    assert(result_hash != initial_hash);
    assert(strcmp(result_hash, initial_hash) == 0); // Result should just be the initial hash

    free(result_hash);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

// Tests concatenation when no nodes match the specified type.
void test_no_match_concat() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);
    string_proc_list_add_node_impl(list, 1, "type1");
    string_proc_list_add_node_impl(list, 2, "type2");
    string_proc_list_add_node_impl(list, 3, "type3");

    char* initial_hash = "no_match:";
    char* result_hash = string_proc_list_concat_impl(list, 0, initial_hash); // Request type 0, none exist

    assert(result_hash != NULL);
    assert(result_hash != initial_hash);
    assert(strcmp(result_hash, initial_hash) == 0); // Result should just be the initial hash

    free(result_hash);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

// Tests behavior with NULL inputs for various functions.
void test_null_inputs() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);
    char* some_hash = "some_hash";
    char* result_hash = NULL;
    string_proc_node* node = NULL;

    printf("  Testing create_node with NULL hash...\n");
    node = string_proc_node_create_impl(1, NULL); // Create node with NULL hash data
    assert(node != NULL);
    if (node != NULL) {
        assert(node->hash == NULL);
        string_proc_node_destroy(node);
    }
    printf("  ...create_node with NULL hash finished.\n");


    printf("  Testing add_node with NULL list...\n");
    // Should handle NULL list gracefully (e.g., do nothing or check internally)
    string_proc_list_add_node_impl(NULL, 0, some_hash);
    printf("  ...add_node with NULL list finished (no crash is good).\n");

    printf("  Testing add_node with NULL hash...\n");
    string_proc_list_add_node_impl(list, 1, NULL); // Add a node with NULL hash data
    assert(list->first != NULL);
    assert(list->last == list->first);
    assert(list->first->type == 1);
    assert(list->first->hash == NULL); // Verify the added node's hash is NULL
    printf("  ...add_node with NULL hash finished.\n");


    printf("  Testing concat with NULL list...\n");
    // Concatenation on a NULL list should return NULL
    result_hash = string_proc_list_concat_impl(NULL, 0, some_hash);
    assert(result_hash == NULL);
    printf("  ...concat with NULL list finished.\n");

    printf("  Testing concat with NULL initial hash...\n");
    string_proc_list_add_node_impl(list, 1, "valid_hash"); // Add another node first
    // Concatenation with NULL initial hash should return NULL
    result_hash = string_proc_list_concat_impl(list, 1, NULL);
    assert(result_hash == NULL);
    printf("  ...concat with NULL initial hash finished.\n");

    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

// Tests adding and concatenating nodes with empty string hashes.
void test_empty_hash_add_concat() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);
    string_proc_list_add_node_impl(list, 0, "first");
    string_proc_list_add_node_impl(list, 0, ""); // Add node with empty string hash
    string_proc_list_add_node_impl(list, 0, "last");

    char* initial_hash = "empty_hash_test:";
    char* result_hash = string_proc_list_concat_impl(list, 0, initial_hash);

    assert(result_hash != NULL);
    // Empty string should be concatenated correctly (effectively ignored)
    assert(strcmp(result_hash, "empty_hash_test:firstlast") == 0);

    free(result_hash);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

// Tests detection of cycles in the list during concatenation.
void test_cycle_detection() {
    printf("Running test: %s\n", __func__);
    string_proc_list * list = string_proc_list_create_impl();
    assert(list != NULL);

    string_proc_list_add_node_impl(list, 0, "a");
    string_proc_list_add_node_impl(list, 0, "b");
    string_proc_list_add_node_impl(list, 0, "c");

    assert(list->first != NULL && list->last != NULL && list->last->next == NULL);

    printf("  Forcing cycle...\n");
    list->last->next = list->first; // Manually create a cycle

    // Concatenation should detect the cycle and return NULL to avoid infinite loop
    char* result = string_proc_list_concat_impl(list, 0, "cycle_test:");

    assert(result == NULL);
    printf("  Concat returned %s (expected NULL for cycle detection)\n", result == NULL ? "NULL" : "non-NULL");


    // Attempt to safely clean up if cycle detection worked
    if (result == NULL) {
         printf("  Manually breaking cycle before freeing (EXPERIMENTAL)...\n");
         if (list->last != NULL) {
               list->last->next = NULL; // Break the cycle
               string_proc_list_destroy(list); // Now destroy should work
         } else {
              // Should not happen with the setup above, but safety check
              printf("  Cannot break cycle, list->last is NULL. Memory will leak.\n");
         }
    } else {
         // If concat didn't return NULL, something is wrong. Avoid destroying potentially corrupt list.
         printf("  Concat did not return NULL. Potential infinite loop avoided or test failed.\n");
         printf("  Skipping destroy due to unexpected concat result.\n");
    }
    printf("Finished test: %s (Note potential memory leak if cycle handling failed)\n", __func__);
}

// Tests concatenation where the result would exceed the maximum allowed length.
void test_concat_overflow() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);

    // Create a very large string
    char* big_string = malloc(MAX_RESULT_LEN);
    assert(big_string != NULL);
    memset(big_string, 'A', MAX_RESULT_LEN - 1);
    big_string[MAX_RESULT_LEN - 1] = '\0';

    string_proc_list_add_node_impl(list, 0, "small_prefix");
    string_proc_list_add_node_impl(list, 0, big_string); // Add the large string

    char* initial_hash = "B"; // Small initial hash

    // Concatenation should detect potential overflow and return NULL
    char* result = string_proc_list_concat_impl(list, 0, initial_hash);

    assert(result == NULL);
    printf("  Concat returned %s (expected NULL for overflow condition)\n", result == NULL ? "NULL" : "non-NULL");

    free(big_string);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

// Tests the correctness of concatenation with mixed types.
void test_concat_correctness() {
    printf("Running test: %s\n", __func__);
    string_proc_list * list = string_proc_list_create_impl();
    assert(list != NULL);

    string_proc_list_add_node_impl(list, 1, "abc");
    string_proc_list_add_node_impl(list, 1, "def");
    string_proc_list_add_node_impl(list, 2, "ghi"); // Node of different type

    // Concatenate only nodes of type 1
    char* result = string_proc_list_concat_impl(list, 1, "hash-");

    assert(result != NULL);
    assert(strcmp(result, "hash-abcdef") == 0); // Verify only type 1 nodes were concatenated

    free(result);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

// Runs all defined test cases.
void run_tests(){
    printf("==================== Starting Basic Tests ====================\n");
    test_create_destroy_list();
    test_create_destroy_node();
    test_create_list_add_nodes();
    test_list_concat_basic();
    printf("==================== Finished Basic Tests ====================\n\n");

    printf("================ Starting Edge Case Tests =============\n");
    test_empty_list_concat();
    test_no_match_concat();
    test_null_inputs();
    test_empty_hash_add_concat();
    test_concat_correctness();
    test_cycle_detection();
    test_concat_overflow();
    printf("================ Finished  Edge Case Tests =============\n");
}

int main (void){
    run_tests();
    printf("\nAll main tests completed.\n");
    return 0;
}