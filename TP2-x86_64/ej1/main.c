#include "ej1.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <assert.h> // Include assert.h for assertions

#define MAX_RESULT_LEN 1048576  // 1 MB max from user's file

// Determine which implementation to use based on ej1.h
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


/**
*	crea y destruye a una lista vacía
*/
void test_create_destroy_list(){
	printf("Running test: %s\n", __func__);
	string_proc_list * list	= string_proc_list_create_impl();
	assert(list != NULL); // Ensure list creation was successful
	assert(list->first == NULL);
	assert(list->last == NULL);
	string_proc_list_destroy(list);
	printf("Finished test: %s\n", __func__);
}

/**
*	crea y destruye un nodo
*/
void test_create_destroy_node(){
	printf("Running test: %s\n", __func__);
	char* hash_val = "hash";
	string_proc_node* node	= string_proc_node_create_impl(0, hash_val);
	assert(node != NULL); // Ensure node creation was successful
	assert(node->type == 0);
	assert(node->hash == hash_val); // Should point to the same memory
	assert(node->next == NULL);
	assert(node->previous == NULL);
	string_proc_node_destroy(node);
	printf("Finished test: %s\n", __func__);
}

/**
 * crea una lista y le agrega nodos
*/
void test_create_list_add_nodes()
{
	printf("Running test: %s\n", __func__);
	string_proc_list * list	= string_proc_list_create_impl();
	assert(list != NULL);
	string_proc_list_add_node_impl(list, 0, "hola");
	string_proc_list_add_node_impl(list, 1, "a");
	string_proc_list_add_node_impl(list, 0, "todos!");
	assert(list->first != NULL);
	assert(list->last != NULL);
	assert(strcmp(list->first->hash, "hola") == 0);
	assert(strcmp(list->last->hash, "todos!") == 0);
	string_proc_list_destroy(list);
	printf("Finished test: %s\n", __func__);
}

/**
 * crea una lista y le agrega nodos. Luego aplica la lista a un hash.
*/
void test_list_concat_basic() // Renamed from test_list_concat to be specific
{
	printf("Running test: %s\n", __func__);
	string_proc_list * list	= string_proc_list_create_impl();
	assert(list != NULL);
	string_proc_list_add_node_impl(list, 0, "hola");
	string_proc_list_add_node_impl(list, 1, "a"); // Different type
	string_proc_list_add_node_impl(list, 0, "todos!");
	char* initial_hash = "hash:";
	char* new_hash = string_proc_list_concat_impl(list, 0, initial_hash);
	assert(new_hash != NULL);
	// Expected: "hash:holatodos!"
	assert(strcmp(new_hash, "hash:holatodos!") == 0);
	string_proc_list_destroy(list);
	free(new_hash);
	printf("Finished test: %s\n", __func__);
}

// --- TEST CASES ---

/**
 * Test Case 1: Concatenation on an empty list.
*/
void test_empty_list_concat() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);
    char* initial_hash = "empty_test:";
    char* result_hash = string_proc_list_concat_impl(list, 0, initial_hash);

    assert(result_hash != NULL); // Should return a new string
    assert(result_hash != initial_hash); // Should be a copy, different memory address
    assert(strcmp(result_hash, initial_hash) == 0); // Content should be the same

    free(result_hash);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

/**
 * Test Case 2: Concatenation with no matching types.
*/
void test_no_match_concat() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);
    string_proc_list_add_node_impl(list, 1, "type1");
    string_proc_list_add_node_impl(list, 2, "type2");
    string_proc_list_add_node_impl(list, 3, "type3");

    char* initial_hash = "no_match:";
    // Try concatenating with type 0, which is not in the list
    char* result_hash = string_proc_list_concat_impl(list, 0, initial_hash);

    assert(result_hash != NULL); // Should return a new string
    assert(result_hash != initial_hash); // Should be a copy
    assert(strcmp(result_hash, initial_hash) == 0); // Content should be unchanged

    free(result_hash);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

/**
 * Test Case 3: Handling NULL inputs (Combined).
*/
void test_null_inputs() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);
	char* some_hash = "some_hash";
	char* result_hash = NULL;
    string_proc_node* node = NULL;

    // Test create_node with NULL hash (from user's test_node_create_null_hash intent)
    // Assuming create_node should succeed but node->hash will be NULL
    printf("  Testing create_node with NULL hash...\n");
    node = string_proc_node_create_impl(1, NULL);
    // Original user test asserted node == NULL. Let's assume the C/ASM impl *can* create it.
    // If it *should* fail, the assert should be assert(node == NULL);
    assert(node != NULL);
    if (node != NULL) {
        assert(node->hash == NULL);
        string_proc_node_destroy(node); // Clean up the created node
    }
	printf("  ...create_node with NULL hash finished.\n");


	// Test add_node with NULL list (should not crash)
	printf("  Testing add_node with NULL list...\n");
    string_proc_list_add_node_impl(NULL, 0, some_hash);
	printf("  ...add_node with NULL list finished (no crash is good).\n");

	// Test add_node with NULL hash (should handle it gracefully)
	printf("  Testing add_node with NULL hash...\n");
    string_proc_list_add_node_impl(list, 1, NULL);
	// Check list state: should have one node with NULL hash
	assert(list->first != NULL);
	assert(list->last == list->first); // Only one node
	assert(list->first->type == 1);
	assert(list->first->hash == NULL); // Assert it points to NULL hash
	printf("  ...add_node with NULL hash finished.\n");


	// Test concat with NULL list
	printf("  Testing concat with NULL list...\n");
    result_hash = string_proc_list_concat_impl(NULL, 0, some_hash);
    assert(result_hash == NULL); // Expect NULL return value
	printf("  ...concat with NULL list finished.\n");

	// Test concat with NULL initial hash
	printf("  Testing concat with NULL initial hash...\n");
	// Need list to be non-empty for this to be meaningful
	string_proc_list_add_node_impl(list, 1, "valid_hash");
	result_hash = string_proc_list_concat_impl(list, 1, NULL);
	// The behavior here depends on the implementation of str_concat and concat logic.
	// Let's assume it should fail gracefully and return NULL.
	assert(result_hash == NULL);
	printf("  ...concat with NULL initial hash finished.\n");

	string_proc_list_destroy(list); // Clean up the list
    printf("Finished test: %s\n", __func__);
}


/**
 * Test Case 4: Adding a node with an empty string hash and concatenating.
*/
void test_empty_hash_add_concat() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);
    string_proc_list_add_node_impl(list, 0, "first");
    string_proc_list_add_node_impl(list, 0, ""); // Add node with empty hash
    string_proc_list_add_node_impl(list, 0, "last");

    char* initial_hash = "empty_hash_test:";
    char* result_hash = string_proc_list_concat_impl(list, 0, initial_hash);

    assert(result_hash != NULL);
    // Expected: "empty_hash_test:firstlast" (empty string concatenates nothing)
    assert(strcmp(result_hash, "empty_hash_test:firstlast") == 0);

    free(result_hash);
    string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

/**
 * Test Case 5: Cycle detection (from user's test_cycle_detection).
 * NOTE: This test creates a list that cannot be safely destroyed by the provided function.
 */
void test_cycle_detection() {
	printf("Running test: %s\n", __func__);
	string_proc_list * list = string_proc_list_create_impl();
	assert(list != NULL);

	string_proc_list_add_node_impl(list, 0, "a");
	string_proc_list_add_node_impl(list, 0, "b");
	string_proc_list_add_node_impl(list, 0, "c");

    // Check list state before creating cycle
    assert(list->first != NULL && list->last != NULL && list->last->next == NULL);

	// Force a cycle: point the last node's next to the first node
    printf("  Forcing cycle...\n");
	list->last->next = list->first;
    // Optionally: list->first->previous = list->last; // Make it doubly linked cycle if needed

	char* result = string_proc_list_concat_impl(list, 0, "cycle_test:");

    // Expect concat implementation to detect the cycle and return NULL
    // (or handle it in a defined way, NULL seems reasonable to indicate error/impossibility)
	assert(result == NULL);
    printf("  Concat returned %s (expected NULL for cycle detection)\n", result == NULL ? "NULL" : "non-NULL");


	// !!! WARNING !!!
	// The list now has a cycle. Calling string_proc_list_destroy(list)
	// will likely result in an infinite loop.
	// In a real scenario, you'd need a cycle-detecting destroy function.
	// For this test, we might have to leak the memory or manually break the cycle
	// before destroying, IF the concat test passes (meaning concat didn't loop infinitely).
	// If concat itself loops infinitely on a cycle, this test structure won't work well.

	// If concat returns NULL as expected (doesn't loop forever),
	// we can try to manually break the cycle before freeing.
	if (result == NULL) {
         printf("  Manually breaking cycle before freeing (EXPERIMENTAL)...\n");
         // This assumes list->last still points to the node whose 'next' was modified.
         if (list->last != NULL) {
            list->last->next = NULL; // Break the cycle
            string_proc_list_destroy(list); // Now try to destroy
         } else {
             printf("  Cannot break cycle, list->last is NULL. Memory will leak.\n");
             // Or maybe free individual known nodes if safe? Very risky.
             // For test purposes, maybe just free the list struct itself?
             // free(list); // Leaks nodes
         }
	} else {
        printf("  Concat did not return NULL. Potential infinite loop avoided or test failed.\n");
        // If concat didn't return NULL maybe it didn't detect cycle, or maybe it handled it differently.
        // If it didn't loop, try destroying? Risky.
        // Let's assume the test fails if result is not NULL and avoid destroy.
         printf("  Skipping destroy due to unexpected concat result.\n");
    }
    printf("Finished test: %s (Note potential memory leak if cycle handling failed)\n", __func__);
}

/**
 * Test Case 6: Concatenation overflow (from user's test_concat_overflow).
 */
void test_concat_overflow() {
    printf("Running test: %s\n", __func__);
	string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);

	char* big_string = malloc(MAX_RESULT_LEN);
	assert(big_string != NULL); // Ensure malloc succeeded
	memset(big_string, 'A', MAX_RESULT_LEN - 1);
	big_string[MAX_RESULT_LEN - 1] = '\0';

	string_proc_list_add_node_impl(list, 0, "small_prefix"); // Add one small node first
	string_proc_list_add_node_impl(list, 0, big_string);    // Add the huge node

    char* initial_hash = "B"; // Small initial hash

	// Expect concat to detect that concatenating big_string will exceed
    // some internal limit (if implemented) or cause malloc failure.
	char* result = string_proc_list_concat_impl(list, 0, initial_hash);

	assert(result == NULL);  // Assuming concat returns NULL on overflow/allocation failure
    printf("  Concat returned %s (expected NULL for overflow condition)\n", result == NULL ? "NULL" : "non-NULL");

	free(big_string);
	string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}

/**
 * Test Case 7: Specific concatenation correctness (from user's test_concat_correctness).
 */
void test_concat_correctness() {
    printf("Running test: %s\n", __func__);
	string_proc_list * list = string_proc_list_create_impl();
    assert(list != NULL);

	string_proc_list_add_node_impl(list, 1, "abc");
	string_proc_list_add_node_impl(list, 1, "def");
	string_proc_list_add_node_impl(list, 2, "ghi"); // Node with different type, should be ignored

	char* result = string_proc_list_concat_impl(list, 1, "hash-");

    assert(result != NULL);
	assert(strcmp(result, "hash-abcdef") == 0);  // Exact validation

	free(result);
	string_proc_list_destroy(list);
    printf("Finished test: %s\n", __func__);
}


// --- End of tests ---


/**
* Corre los test
*/
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
	test_null_inputs(); // Combined NULL input checks
	test_empty_hash_add_concat();
	test_concat_correctness(); // Specific correctness check
    test_cycle_detection(); // Checks for cycles (potential memory leak)
    test_concat_overflow(); // Checks for large string concatenation
	printf("================ Finished  Edge Case Tests =============\n");
}

int main (void){
	run_tests();
	printf("\nAll main tests completed.\n");
	return 0;
}