#include "ej1.h"
#include <stdlib.h> // For malloc and free
#include <string.h> // For strlen, strcpy, strcat
#include <stdio.h>  // For printf in destroy/print error messages
#include <stdbool.h>// For bool type

// --- Helper function for cycle detection ---

/**
 * @brief Detects if a list contains a cycle using Floyd's Tortoise and Hare algorithm.
 *
 * @param list Pointer to the list.
 * @return true if a cycle is detected, false otherwise.
 */
bool string_proc_list_has_cycle(string_proc_list* list) {
    if (list == NULL || list->first == NULL) {
        return false; // Empty list or no nodes
    }

    string_proc_node *slow = list->first;
    string_proc_node *fast = list->first;

    // Traverse the list with two pointers at different speeds
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;          // Move slow pointer one step
        fast = fast->next->next;    // Move fast pointer two steps

        if (slow == fast) {
            return true; // Pointers met, cycle detected
        }
    }

    return false; // Fast pointer reached end, no cycle
}


// --- Functions to Implement ---

/**
 * @brief Initializes an empty list structure.
 * @return pointer to the newly created list, or NULL if malloc fails.
 */
string_proc_list* string_proc_list_create(void){
    // Allocate memory for the list structure
    string_proc_list* new_list = (string_proc_list*)malloc(sizeof(string_proc_list));
    if (new_list == NULL) {
        // Memory allocation failed
        return NULL;
    }
    // Initialize pointers to NULL for an empty list
    new_list->first = NULL;
    new_list->last = NULL;
    return new_list;
}

/**
 * @brief Initializes a node with the given type and hash.
 * @param type The type of the node.
 * @param hash Pointer to the hash string. The node does NOT own this string.
 * @return pointer to the newly created node, or NULL if malloc fails.
 */
string_proc_node* string_proc_node_create(uint8_t type, char* hash){
    // Allocate memory for the node structure
    string_proc_node* new_node = (string_proc_node*)malloc(sizeof(string_proc_node));
    if (new_node == NULL) {
        // Memory allocation failed
        return NULL;
    }
    // Initialize pointers and values
    new_node->next = NULL;
    new_node->previous = NULL;
    new_node->type = type;
    new_node->hash = hash; // Point to the existing hash string
    return new_node;
}

/**
 * @brief Adds a new node with the given type and hash to the end of the list.
 * @param list Pointer to the list where the node will be added.
 * @param type The type of the new node.
 * @param hash Pointer to the hash string for the new node.
 */
void string_proc_list_add_node(string_proc_list* list, uint8_t type, char* hash){
    if (list == NULL) {
        return; // Do not operate on a NULL list
    }
    // Create the new node
    string_proc_node* new_node = string_proc_node_create(type, hash);
    if (new_node == NULL) {
        // Node creation failed, cannot add
        return;
    }

    // Check if the list is empty
    if (list->last == NULL) {
        // List is empty, the new node is the first and last
        list->first = new_node;
        list->last = new_node;
    } else {
        // List is not empty, add to the end
        list->last->next = new_node;
        new_node->previous = list->last;
        list->last = new_node; // Update the last pointer
    }
}

/**
 * @brief Generates a new hash by concatenating the initial hash
 * with all hashes from nodes in the list whose types match the specified type.
 * Detects cycles and potential result size overflow.
 *
 * @param list Pointer to the list.
 * @param type The type of nodes to search for concatenation.
 * @param hash Pointer to the initial hash string.
 * @return Pointer to a NEW string (created with malloc) containing the concatenation.
 * The caller is responsible for freeing this memory with free().
 * Returns NULL if malloc fails, if the list is NULL, if the initial hash is NULL,
 * if a cycle is detected in the list, or if the concatenation would exceed MAX_RESULT_LEN.
 * Returns a copy of the initial hash if no matching nodes are found (and no other error occurs).
 */
char* string_proc_list_concat(string_proc_list* list, uint8_t type , char* hash){
    if (list == NULL || hash == NULL) {
        return NULL; // Invalid input
    }

    // --- Cycle Detection ---
    // Check for cycles before proceeding to avoid infinite loops
    if (string_proc_list_has_cycle(list)) {
        fprintf(stderr, "Error: Cycle detected in list during concat. Aborting.\n");
        return NULL;
    }
    // --- End Cycle Detection ---


    // Create an initial copy of the hash to start concatenation
    char* result_hash = str_concat("", hash); // Use helper to allocate new memory
    if (result_hash == NULL){
         // Could fail if initial hash itself is too large or malloc fails immediately.
         fprintf(stderr, "Error: Failed to allocate initial result hash.\n");
         return NULL;
    }

    size_t current_len = strlen(result_hash); // Use size_t for string lengths

    // Proceed with concatenation since no cycle was detected
    string_proc_node* current_node = list->first;
    while (current_node != NULL) {
        // Check if the node's type matches the requested type
        if (current_node->type == type) {
            // Only process nodes with non-NULL hashes
            if (current_node->hash != NULL) {
                size_t len_node_hash = strlen(current_node->hash);

                // --- Overflow Check ---
                // Check if adding the next hash exceeds the predefined limit.
                // Required: current_len + len_node_hash + 1 (for null terminator) <= MAX_RESULT_LEN
                // Safer check to avoid potential intermediate overflow:
                if (len_node_hash >= MAX_RESULT_LEN - current_len) {
                     fprintf(stderr, "Error: Concatenation would exceed MAX_RESULT_LEN. Aborting.\n");
                     free(result_hash); // Free the partially built string
                     return NULL;
                }
                // --- End Overflow Check ---


                // If check passes, proceed with concatenation
                char* temp_hash = str_concat(result_hash, current_node->hash);
                if (temp_hash == NULL) {
                    // Malloc likely failed inside str_concat
                    fprintf(stderr, "Error: str_concat failed during concatenation (malloc failed?). Aborting.\n");
                    free(result_hash); // Free the previous result string
                    return NULL;
                }
                // Free the previous result hash string
                free(result_hash);
                // Update the pointer to the new concatenated hash string
                result_hash = temp_hash;
                // Update the current length
                current_len += len_node_hash; // Keep track of the total length
            }
            // Else: Skip node if its hash is NULL
        }
        current_node = current_node->next; // Move to the next node
    }

    return result_hash; // Return the final concatenated string
}

/**
 * ============================================================================
 * AUXILIARY FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Concatenates two strings a and b.
 * @param a First string.
 * @param b Second string.
 * @return Pointer to a NEW string (created with malloc) containing a followed by b.
 * Returns NULL if malloc fails or if either input string is NULL.
 * Caller must free the returned string.
 */
char* str_concat(char* a, char* b) {
    if (a == NULL || b == NULL) return NULL; // Check for NULL inputs

    size_t len1 = strlen(a);
    size_t len2 = strlen(b);
    // Check for potential overflow when calculating total length
    if (len1 > SIZE_MAX - len2) {
        fprintf(stderr, "Error: Size overflow during length calculation in str_concat.\n");
        return NULL;
    }
    size_t totalLength = len1 + len2;

    char *result = (char *)malloc(totalLength + 1); // +1 for the null terminator
    if (result == NULL) {
        // Malloc failed
        return NULL;
    }
    strcpy(result, a); // Copy first string
    strcat(result, b); // Append second string
    return result;
}

/**
 * @brief Frees the memory of the list and all its nodes. Detects cycles.
 * NOTE: Does NOT free the memory of the 'hash' strings pointed to by the nodes!
 * If a cycle is detected, it stops freeing nodes to prevent an infinite loop,
 * potentially leaking the nodes within the cycle. It always frees the list structure itself.
 * @param list Pointer to the list to destroy.
*/
void string_proc_list_destroy(string_proc_list* list){
    if (list == NULL) return; // Nothing to destroy

    // --- Cycle Detection ---
    // Check for cycles before iterating to free nodes
    if (string_proc_list_has_cycle(list)) {
        fprintf(stderr, "Warning: Cycle detected in list during destroy. Nodes in cycle will not be freed to prevent infinite loop.\n");
        // Free only the list structure itself, as iterating through nodes is unsafe.
        free(list);
        return;
    }
    // --- End Cycle Detection ---

    // No cycle detected, proceed with normal node-by-node destruction
    string_proc_node* current_node  = list->first;
    string_proc_node* next_node     = NULL;
    while(current_node != NULL){ // This loop is now safe
        next_node = current_node->next;     // Store next node before freeing current
        string_proc_node_destroy(current_node); // Free the current node structure
        current_node = next_node;           // Move to the next node
    }

    list->first = NULL; // Optional: Clear pointers after freeing nodes
    list->last  = NULL;
    free(list); // Finally, free the list structure itself
}

/**
 * @brief Frees the memory of a single node structure.
 * Does NOT free the 'hash' string pointer.
 * @param node Pointer to the node to destroy.
*/
void string_proc_node_destroy(string_proc_node* node){
    if (node == NULL) return;
    // Optional: Nullify pointers before freeing, can help catch use-after-free
    node->next      = NULL;
    node->previous  = NULL;
    node->hash      = NULL; // Node doesn't own hash, just clear the pointer
    node->type      = 0;
    free(node); // Free the node structure itself
}

/**
 * @brief Prints the list content to the specified file stream. Detects cycles.
 * If a cycle is detected, prints a warning and a limited number of nodes.
 * @param list Pointer to the list.
 * @param file File stream to print to (e.g., stdout, stderr).
*/
void string_proc_list_print(string_proc_list* list, FILE* file){
    if (list == NULL || file == NULL) return; // Invalid inputs

    // --- Cycle Detection using Tortoise and Hare ---
    string_proc_node* slow = list->first;
    string_proc_node* fast = list->first;
    bool cycle_detected = false;

    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) {
            cycle_detected = true;
            break; // Cycle found
        }
    }
    // --- End Cycle Detection ---

    if (cycle_detected) {
         fprintf(file, "List contains a cycle! Printing limited nodes:\n");
         string_proc_node* current_node = list->first;
         int print_limit = 20; // Limit printing to avoid excessive output in long cycles
         while(current_node != NULL && print_limit > 0) {
              fprintf(file, "\tnode hash: %s | type: %d (Potential Cycle)\n",
                        (current_node->hash ? current_node->hash : "NULL"), // Handle NULL hash
                        current_node->type);
              current_node = current_node->next;
              print_limit--;
              // Check if we re-visited the start or detected node to break early
              if (current_node == slow || current_node == list->first) {
                   if (print_limit < 19) { // Avoid printing if cycle detected immediately
                        fprintf(file, "\t...Cycle detected during printing...\n");
                        break;
                   }
              }
         }
         if (print_limit == 0 && current_node != NULL) {
              fprintf(file, "\t...Print limit reached (Potential Cycle)...\n");
         }
    } else {
        // No cycle detected, print normally
        uint32_t length = 0;
        string_proc_node* current_node = list->first;
        // First pass to count length
        while(current_node != NULL){
            length++;
            current_node = current_node->next;
        }
        fprintf( file, "List length: %u\n", length ); // Use %u for uint32_t

        // Second pass to print nodes
        current_node = list->first;
        while(current_node != NULL){
                fprintf(file, "\tnode hash: %s | type: %d\n",
                        (current_node->hash ? current_node->hash : "NULL"), // Handle NULL hash
                        current_node->type);
                current_node = current_node->next;
        }
    }
}