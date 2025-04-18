#ifndef EJ1_H // Include guard: Prevents multiple inclusions of this header file
#define EJ1_H

#include <stdio.h>   // For FILE type in string_proc_list_print
#include <stdlib.h>  // For NULL, malloc/free (implicitly used via functions)
#include <stdint.h>  // For uint8_t type
#include <string.h>  // For string operations (implicitly used via functions)
#include <assert.h>  // For assert (used in tests)
#include <stdbool.h> // For bool type

// Define USE_ASM_IMPL to 1 to enable assembly implementations, 0 for C implementations
#define USE_ASM_IMPL 1

// Define the maximum allowed length for the result of string_proc_list_concat
#define MAX_RESULT_LEN 1048576  // 1 MiB maximum result size

/** List Structure **/
typedef struct string_proc_list_t {
    struct string_proc_node_t* first; // Pointer to the first node in the list
    struct string_proc_node_t* last;  // Pointer to the last node in the list
} string_proc_list;

/** Node Structure **/
typedef struct string_proc_node_t {
    struct string_proc_node_t* next;     // Pointer to the next node
    struct string_proc_node_t* previous; // Pointer to the previous node (for doubly linked list)
    uint8_t type;                        // Type identifier for the node
    char* hash;                          // Pointer to the hash string (memory managed externally)
} string_proc_node;

/** Functions to implement (C and ASM versions): **/

// Creates and initializes an empty list.
string_proc_list* string_proc_list_create(void);
string_proc_list* string_proc_list_create_asm(void);

// Creates and initializes a list node. Does not allocate memory for the hash string.
string_proc_node* string_proc_node_create(uint8_t type, char* hash);
string_proc_node* string_proc_node_create_asm(uint8_t type, char* hash);

// Adds a new node to the end of the list.
void string_proc_list_add_node(string_proc_list* list, uint8_t type, char* hash);
void string_proc_list_add_node_asm(string_proc_list* list, uint8_t type, char* hash);

// Concatenates hashes from nodes of a specific type. Returns a new string (caller must free). Handles cycles and size limits.
char* string_proc_list_concat(string_proc_list* list, uint8_t type, char* hash);
char* string_proc_list_concat_asm(string_proc_list* list, uint8_t type, char* hash);

/** Auxiliary Functions: **/

/**
 * @brief Frees the memory of the list structure and all its node structures.
 * Detects cycles to prevent infinite loops (may leak nodes in cycle).
 * Does NOT free the 'hash' strings pointed to by the nodes.
 * @param list Pointer to the list to destroy.
*/
void string_proc_list_destroy(string_proc_list* list);

/**
 * @brief Frees the memory of a single node structure.
 * Does NOT free the 'hash' string pointed to by the node.
 * @param node Pointer to the node to destroy.
*/
void string_proc_node_destroy(string_proc_node* node);

/**
 * @brief Concatenates two strings a and b.
 * @return A pointer to a NEW string (created via malloc) containing the result.
 * Caller is responsible for freeing the returned string. Returns NULL on failure.
*/
char* str_concat(char* a, char* b);

/**
 * @brief Prints the list content to the specified file stream. Detects cycles.
 * @param list Pointer to the list.
 * @param file File stream to print to (e.g., stdout).
*/
void string_proc_list_print(string_proc_list* list, FILE* file);

/**
 * @brief Detects if a list contains a cycle using Floyd's Tortoise and Hare algorithm.
 * @param list Pointer to the list.
 * @return true if a cycle is detected, false otherwise.
 */
bool string_proc_list_has_cycle(string_proc_list* list); // Declaration

#endif // EJ1_H - End of include guard