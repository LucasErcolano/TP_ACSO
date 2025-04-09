#include "ej1.h"
#include <stdlib.h> // Para malloc y free
#include <string.h> // Para strlen, strcpy, strcat

// --- Funciones a Implementar ---

/**
 * @brief Inicializa una estructura de lista vacía.
 *
 * @return puntero a la nueva lista creada, o NULL si falla malloc.
 */
string_proc_list* string_proc_list_create(void){
    // Reservar memoria para la estructura de la lista
    string_proc_list* new_list = (string_proc_list*)malloc(sizeof(string_proc_list));
    if (new_list == NULL) {
        // Falló la asignación de memoria
        return NULL;
    }
    // Inicializar los punteros a NULL
    new_list->first = NULL;
    new_list->last = NULL;
    return new_list;
}

/**
 * @brief Inicializa un nodo con el tipo y el hash dado.
 * El nodo apunta al hash pasado por parámetro (no lo copia).
 *
 * @param type El tipo del nodo.
 * @param hash Puntero al string del hash.
 * @return puntero al nuevo nodo creado, o NULL si falla malloc.
 */
string_proc_node* string_proc_node_create(uint8_t type, char* hash){
    // Reservar memoria para la estructura del nodo
    string_proc_node* new_node = (string_proc_node*)malloc(sizeof(string_proc_node));
    if (new_node == NULL) {
        // Falló la asignación de memoria
        return NULL;
    }
    // Inicializar los punteros y valores
    new_node->next = NULL;
    new_node->previous = NULL;
    new_node->type = type;
    new_node->hash = hash; // Apuntar al hash existente
    return new_node;
}

/**
 * @brief Agrega un nodo nuevo al final de la lista con el tipo y el hash dado.
 * El nodo apunta al hash pasado por parámetro (no lo copia).
 *
 * @param list Puntero a la lista donde agregar el nodo.
 * @param type El tipo del nuevo nodo.
 * @param hash Puntero al string del hash para el nuevo nodo.
 */
void string_proc_list_add_node(string_proc_list* list, uint8_t type, char* hash){
    if (list == NULL) {
        return; // No operar sobre una lista nula
    }
    // Crear el nuevo nodo
    string_proc_node* new_node = string_proc_node_create(type, hash);
    if (new_node == NULL) {
        // Falló la creación del nodo, no se puede agregar
        // (Podríamos manejar el error de otra forma, pero por ahora salimos)
        return;
    }

    // Verificar si la lista está vacía
    if (list->last == NULL) {
        // La lista está vacía, el nuevo nodo es el primero y el último
        list->first = new_node;
        list->last = new_node;
    } else {
        // La lista no está vacía, agregar al final
        list->last->next = new_node;    // El siguiente del último actual es el nuevo nodo
        new_node->previous = list->last; // El anterior del nuevo nodo es el último actual
        list->last = new_node;          // Actualizar el puntero al último nodo de la lista
    }
}

/**
 * @brief Genera un nuevo hash concatenando el hash pasado por parámetro
 * con todos los hashes de los nodos de la lista cuyos tipos
 * coinciden con el tipo pasado por parámetro.
 *
 * @param list Puntero a la lista.
 * @param type El tipo de nodos a buscar para concatenar.
 * @param hash Puntero al string del hash inicial.
 * @return Puntero a un NUEVO string (creado con malloc) que contiene la concatenación.
 * El llamador es responsable de liberar esta memoria con free().
 * Retorna NULL si falla malloc o si la lista es NULL.
 * Retorna una copia del hash inicial si no hay nodos coincidentes.
 */
char* string_proc_list_concat(string_proc_list* list, uint8_t type , char* hash){
    if (list == NULL || hash == NULL) {
        return NULL;
    }

    // Crear una copia inicial del hash para empezar la concatenación
    // Usamos str_concat("", hash) para obtener una copia fresca vía malloc
    char* result_hash = str_concat("", hash); // str_concat debería manejar hash NULL si es necesario
    if (result_hash == NULL){
         return NULL; // Fallo en malloc dentro de str_concat
    }


    string_proc_node* current_node = list->first;
    while (current_node != NULL) {
        if (current_node->type == type) {
            // El tipo coincide, concatenar
            char* temp_hash = str_concat(result_hash, current_node->hash);
            if (temp_hash == NULL) {
                 // Fallo en malloc dentro de str_concat, liberar memoria previa y retornar NULL
                 free(result_hash);
                 return NULL;
            }
            // Liberar el hash resultado anterior
            free(result_hash);
            // Actualizar el puntero al nuevo hash resultado
            result_hash = temp_hash;
        }
        current_node = current_node->next;
    }

    return result_hash;
}


/**
 * ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================
 * (No modificar las existentes)
*/

/**
 * @brief Concatena dos strings a y b.
 * Retorna el resultado en un nuevo string creado vía malloc.
 * El llamador es responsable de liberar esta memoria.
*/
char* str_concat(char* a, char* b) {
    if (a == NULL || b == NULL) return NULL; // Basic check

	int len1 = strlen(a);
    int len2 = strlen(b);
	int totalLength = len1 + len2;
    // Reservar memoria para el nuevo string (longitud total + 1 para el terminador nulo)
    char *result = (char *)malloc(totalLength + 1);
    if (result == NULL) {
        // Falló la asignación de memoria
        return NULL;
    }
    // Copiar el primer string
    strcpy(result, a);
    // Concatenar el segundo string
    strcat(result, b);
    return result;
}

/**
 * @brief Libera la memoria de la lista y todos sus nodos.
 * ¡No libera la memoria de los strings 'hash' apuntados por los nodos!
*/
void string_proc_list_destroy(string_proc_list* list){
    if (list == NULL) return;
	/* borro los nodos: */
	string_proc_node* current_node	= list->first;
	string_proc_node* next_node		= NULL;
	while(current_node != NULL){
		next_node = current_node->next;
		string_proc_node_destroy(current_node); // Libera el nodo en sí
		current_node = next_node;
	}
	/*borro la lista:*/
	list->first = NULL;
	list->last  = NULL;
	free(list); // Libera la estructura de la lista
}

/**
 * @brief Libera la memoria de un nodo.
 * ¡No libera la memoria del string 'hash' apuntado por el nodo!
*/
void string_proc_node_destroy(string_proc_node* node){
    if (node == NULL) return;
	node->next      = NULL;
	node->previous	= NULL;
	// No hacemos free(node->hash) porque solo apuntamos a él
	node->hash		= NULL;
	node->type      = 0;
	free(node); // Libera la estructura del nodo
}

/**
 * @brief Imprime la lista list en el archivo file.
*/
void string_proc_list_print(string_proc_list* list, FILE* file){
        if (list == NULL || file == NULL) return;
        uint32_t length = 0;
        string_proc_node* current_node  = list->first;
        while(current_node != NULL){
                length++;
                current_node = current_node->next;
        }
        fprintf( file, "List length: %d\n", length );
		current_node    = list->first;
        while(current_node != NULL){
                fprintf(file, "\tnode hash: %s | type: %d\n",
                        (current_node->hash ? current_node->hash : "NULL"), // Check for NULL hash
                        current_node->type);
                current_node = current_node->next;
        }
}