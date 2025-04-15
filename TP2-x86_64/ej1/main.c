#include "ej1.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <assert.h> // Include assert.h for assertions

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
void test_list_concat_basic()
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

// --- NEW TEST CASES ---

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
 * Test Case 3: Handling NULL inputs.
*/
void test_null_inputs() {
    printf("Running test: %s\n", __func__);
    string_proc_list* list = string_proc_list_create_impl();
    assert(list != NULL);
		char* some_hash = "some_hash";
		char* result_hash = NULL;

		// Test add_node with NULL list (should not crash)
		printf("  Testing add_node with NULL list...\n");
    string_proc_list_add_node_impl(NULL, 0, some_hash);
		printf("  ...add_node with NULL list finished (no crash is good).\n");

		// Test add_node with NULL hash (should ideally handle it gracefully, e.g., add node pointing to NULL)
		printf("  Testing add_node with NULL hash...\n");
    string_proc_list_add_node_impl(list, 1, NULL);
		// Add assertion here if specific behavior is expected (e.g., list->last->hash == NULL)
		// For now, just check it doesn't crash and list might contain the node.
		assert(list->first != NULL); // List should have one node now
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
		result_hash = string_proc_list_concat_impl(list, 1, NULL);
		assert(result_hash == NULL); // Expect NULL return value (or handle as per implementation spec)
		printf("  ...concat with NULL initial hash finished.\n");

		string_proc_list_destroy(list); // Clean up the list with the NULL hash node
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


// --- End of NEW TEST CASES ---


/**
* Corre los test a se escritos por lxs alumnxs
*/
void run_tests(){
	printf("==================== Starting Basic Tests ====================\n");
	test_create_destroy_list();
	test_create_destroy_node();
	test_create_list_add_nodes();
	test_list_concat_basic();
	printf("==================== Finished Basic Tests ====================\n\n");

	printf("================ Starting New Edge Case Tests ================\n");
	test_empty_list_concat();
	test_no_match_concat();
	test_null_inputs();
	test_empty_hash_add_concat();
	printf("================ Finished New Edge Case Tests ================\n");
}

int main (void){
	run_tests();
	printf("\nAll main tests completed.\n");
	return 0;
}