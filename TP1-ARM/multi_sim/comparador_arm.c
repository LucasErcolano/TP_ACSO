#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_COMMAND_LENGTH 256
#define BUFFER_SIZE 4096

typedef struct {
    char *name;
    FILE *input;
    FILE *output;
    pid_t pid;
} Simulator;

int initialize_simulator(Simulator *sim, const char *path, const char *program_file) {
    int stdin_pipe[2];
    int stdout_pipe[2];
    
    if (pipe(stdin_pipe) {
        perror("Error creando pipes");
        return -1;
    }
    if (pipe(stdout_pipe)) {
        perror("Error creando pipes");
        return -1;
    }
    
    sim->pid = fork();
    if (sim->pid < 0) {
        perror("Error en fork");
        return -1;
    } else if (sim->pid == 0) {
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        execlp(path, path, program_file, NULL);
        perror("Error ejecutando simulador");
        exit(EXIT_FAILURE);
    } else {
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        sim->input = fdopen(stdin_pipe[1], "w");
        sim->output = fdopen(stdout_pipe[0], "r");
        
        if (!sim->input || !sim->output) {
            perror("Error creando streams");
            return -1;
        }
        
        setbuf(sim->input, NULL);
        
        // Leer hasta encontrar el prompt "ARM-SIM>"
        char buffer[BUFFER_SIZE];
        int found_prompt = 0;
        while (!found_prompt) {
            if (fgets(buffer, BUFFER_SIZE, sim->output) == NULL) {
                perror("Error leyendo salida inicial del simulador");
                return -1;
            }
            if (strstr(buffer, "ARM-SIM>") != NULL) {
                found_prompt = 1;
            }
        }
        return 0;
    }
}

// [Las funciones send_command, cleanup_simulator, print_differences permanecen igual]

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <simulador1> <programa1.x> <simulador2> <programa2.x>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    Simulator sim1 = {.name = "Simulador 1", .input = NULL, .output = NULL, .pid = -1};
    Simulator sim2 = {.name = "Simulador 2", .input = NULL, .output = NULL, .pid = -1};
    
    printf("Inicializando simuladores...\n");
    if (initialize_simulator(&sim1, argv[1], argv[2]) != 0) {
        fprintf(stderr, "Error inicializando %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    if (initialize_simulator(&sim2, argv[3], argv[4]) != 0) {
        fprintf(stderr, "Error inicializando %s\n", argv[3]);
        cleanup_simulator(&sim1);
        return EXIT_FAILURE;
    }
    
    printf("Simuladores inicializados correctamente.\n");
    printf("Ingrese comandos (o 'exit' para salir):\n");
    
    char command[MAX_COMMAND_LENGTH];
    while (1) {
        printf("Comando> ");
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) break;
        
        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "exit") == 0) break;
        
        printf("\nEjecutando: %s\n", command);
        printf("--- SIM1 ---\n");
        char *output1 = send_command(&sim1, command);
        printf("%s", output1);
        
        printf("--- SIM2 ---\n");
        char *output2 = send_command(&sim2, command);
        printf("%s", output2);
        
        if (strcmp(output1, output2) {
            print_differences(output1, output2);
        } else {
            printf("\n✓ Salidas coinciden\n");
        }
    }
    
    printf("Terminando...\n");
    cleanup_simulator(&sim1);
    cleanup_simulator(&sim2);
    return EXIT_SUCCESS;
}