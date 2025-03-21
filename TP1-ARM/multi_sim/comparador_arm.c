#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_COMMAND_LENGTH 256
#define BUFFER_SIZE 4096

// Estructura para almacenar información de un simulador
typedef struct {
    char *name;
    FILE *input;
    FILE *output;
    pid_t pid;
} Simulator;

// Inicializa un simulador, creando su proceso y configurando pipes
int initialize_simulator(Simulator *sim, const char *path) {
    int stdin_pipe[2];  // Para enviar comandos al simulador
    int stdout_pipe[2]; // Para recibir salida del simulador
    
    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0) {
        perror("Error creando pipes");
        return -1;
    }
    
    // Crear proceso hijo para el simulador
    sim->pid = fork();
    if (sim->pid < 0) {
        perror("Error en fork");
        return -1;
    } else if (sim->pid == 0) {
        // Proceso hijo (simulador)
        close(stdin_pipe[1]);  // Cerrar extremo de escritura de stdin
        close(stdout_pipe[0]); // Cerrar extremo de lectura de stdout
        
        // Redirigir stdin y stdout
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        
        // Cerrar duplicados
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        // Ejecutar el simulador
        execlp(path, path, NULL);
        perror("Error ejecutando simulador");
        exit(EXIT_FAILURE);
    } else {
        // Proceso padre
        close(stdin_pipe[0]);  // Cerrar extremo de lectura de stdin
        close(stdout_pipe[1]); // Cerrar extremo de escritura de stdout
        
        // Configurar streams para comunicación con el simulador
        sim->input = fdopen(stdin_pipe[1], "w");
        sim->output = fdopen(stdout_pipe[0], "r");
        
        if (!sim->input || !sim->output) {
            perror("Error creando streams");
            return -1;
        }
        
        // Configurar buffer para salida inmediata
        setbuf(sim->input, NULL);
        
        // Leer el mensaje de inicio del simulador (banner)
        char buffer[BUFFER_SIZE];
        if (fgets(buffer, BUFFER_SIZE, sim->output) == NULL) {
            perror("Error leyendo banner del simulador");
            return -1;
        }
        
        return 0;
    }
}

// Envía un comando a un simulador y captura su salida
char* send_command(Simulator *sim, const char *command) {
    static char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    // Enviar comando al simulador
    fprintf(sim->input, "%s\n", command);
    fflush(sim->input);
    
    // Leer la respuesta hasta el prompt "ARM-SIM> "
    char *output_ptr = buffer;
    char line[BUFFER_SIZE];
    size_t remaining = BUFFER_SIZE - 1;
    
    while (remaining > 0 && fgets(line, sizeof(line), sim->output) != NULL) {
        size_t len = strlen(line);
        
        // Verificar si hemos llegado al prompt
        if (strstr(line, "ARM-SIM>") != NULL) {
            break;
        }
        
        // Copiar la línea al buffer de salida
        if (len < remaining) {
            strcpy(output_ptr, line);
            output_ptr += len;
            remaining -= len;
        } else {
            // No hay suficiente espacio, truncar
            strncpy(output_ptr, line, remaining);
            output_ptr += remaining;
            remaining = 0;
            break;
        }
    }
    
    *output_ptr = '\0';
    return buffer;
}

// Limpia y termina un simulador
void cleanup_simulator(Simulator *sim) {
    if (sim->input) {
        fprintf(sim->input, "quit\n");
        fclose(sim->input);
    }
    
    if (sim->output) {
        fclose(sim->output);
    }
    
    if (sim->pid > 0) {
        int status;
        waitpid(sim->pid, &status, 0);
    }
}

// Imprime las diferencias entre dos salidas
void print_differences(const char *output1, const char *output2) {
    printf("\n--- DIFERENCIAS ENCONTRADAS ---\n");
    
    // Tokenizar las salidas por líneas para comparar
    char *o1 = strdup(output1);
    char *o2 = strdup(output2);
    
    char *line1 = strtok(o1, "\n");
    char *line2 = strtok(o2, "\n");
    
    int line_num = 1;
    
    while (line1 != NULL || line2 != NULL) {
        if (line1 == NULL) {
            printf("Línea %d - Sim2: %s\n", line_num, line2);
        } else if (line2 == NULL) {
            printf("Línea %d - Sim1: %s\n", line_num, line1);
        } else if (strcmp(line1, line2) != 0) {
            printf("Línea %d:\n", line_num);
            printf("  Sim1: %s\n", line1);
            printf("  Sim2: %s\n", line2);
        }
        
        if (line1) line1 = strtok(NULL, "\n");
        if (line2) line2 = strtok(NULL, "\n");
        line_num++;
    }
    
    free(o1);
    free(o2);
    printf("-----------------------------\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <simulador1> <simulador2>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    Simulator sim1 = {.name = "Simulador 1", .input = NULL, .output = NULL, .pid = -1};
    Simulator sim2 = {.name = "Simulador 2", .input = NULL, .output = NULL, .pid = -1};
    
    // Inicializar simuladores
    printf("Inicializando simuladores...\n");
    if (initialize_simulator(&sim1, argv[1]) != 0) {
        fprintf(stderr, "Error inicializando %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    
    if (initialize_simulator(&sim2, argv[2]) != 0) {
        fprintf(stderr, "Error inicializando %s\n", argv[2]);
        cleanup_simulator(&sim1);
        return EXIT_FAILURE;
    }
    
    printf("Simuladores inicializados correctamente.\n");
    printf("Ingrese comandos para enviar a ambos simuladores (o 'exit' para salir):\n");
    
    char command[MAX_COMMAND_LENGTH];
    while (1) {
        printf("Comando> ");
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            break;
        }
        
        // Eliminar el salto de línea final
        size_t len = strlen(command);
        if (len > 0 && command[len-1] == '\n') {
            command[len-1] = '\0';
        }
        
        // Salir si se ingresa 'exit'
        if (strcmp(command, "exit") == 0) {
            break;
        }
        
        // Enviar comando a ambos simuladores
        printf("\nEjecutando: %s\n", command);
        printf("\n--- RESULTADO SIMULADOR 1 ---\n");
        char *output1 = send_command(&sim1, command);
        printf("%s", output1);
        
        printf("\n--- RESULTADO SIMULADOR 2 ---\n");
        char *output2 = send_command(&sim2, command);
        printf("%s", output2);
        
        // Comparar salidas
        if (strcmp(output1, output2) != 0) {
            print_differences(output1, output2);
        } else {
            printf("\n✓ Las salidas son idénticas\n");
        }
    }
    
    // Limpiar y terminar
    printf("Terminando simuladores...\n");
    cleanup_simulator(&sim1);
    cleanup_simulator(&sim2);
    
    return EXIT_SUCCESS;
}