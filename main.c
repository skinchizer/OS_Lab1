#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

// Function prototypes
void on_child_exit(int signum);
void setup_environment();
void shell();
void parse_input(char* input, char* command, char** args, int* background);
void execute_shell_builtin(char* command, char** args);
void execute_command(char* command, char** args, int background);

// Global variables
char cwd[MAX_COMMAND_LENGTH];
FILE* logfile;

int main() {
    // Register signal handler for child exit
    signal(SIGCHLD, on_child_exit);

    // Open log file
    logfile = fopen("shell_log.txt", "a");
    if (logfile == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    // Set up environment
    setup_environment();

    // Start shell
    shell();

    // Close log file
    fclose(logfile);

    return 0;
}

void on_child_exit(int signum) {
    // Reap zombie child processes
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // Write to log file
        fprintf(logfile, "Child process was terminated\n");
    }
}

void setup_environment() {
    // Get current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }
}

void shell() {
    char input[MAX_COMMAND_LENGTH];
    char command[MAX_COMMAND_LENGTH];
    char* args[MAX_ARGS];
    int background;

    do {
        printf("%s$ ", cwd);
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0'; // Remove newline character

        // Parse input
        parse_input(input, command, args, &background);

        // Execute command
        if (strcmp(command, "exit") == 0) {
            break; // Exit shell
        } else if (strcmp(command, "echo") == 0) {
			execute_shell_builtin(command, args);
		} else if (strcmp(command, "cd") == 0) {
			execute_shell_builtin(command, args);
		} else if (strcmp(command, "export") == 0) {
			execute_shell_builtin(command, args);
		} else if (strcmp(command, "") != 0) {
            if (background) {
                // Execute command in background
                execute_command(command, args, 1);
            } else {
                // Execute command in foreground
                execute_command(command, args, 0);
            }
        }
    } while (1);
}

void parse_input(char* input, char* command, char** args, int* background) {
    // Check if command should be executed in background
    *background = (input[strlen(input) - 1] == '&') ? 1 : 0;

    // Tokenize input into command and arguments
    char* token = strtok(input, " ");
    strcpy(command, token);

    int i = 0;
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // Null-terminate args array
}

void execute_shell_builtin(char* command, char** args) {
        if (strcmp(command, "cd") == 0) {
        // Handle 'cd' command
        char* target_dir = args[1];
        if (target_dir == NULL) {
            // Change to home directory
            chdir(getenv("HOME"));
        } else {
            // Handle '~' symbol
            if (target_dir[0] == '~') {
                char* home_dir = getenv("HOME");
                char new_target_dir[MAX_COMMAND_LENGTH];
                snprintf(new_target_dir, sizeof(new_target_dir), "%s%s", home_dir, target_dir + 1);
                target_dir = new_target_dir;
            }
            // Handle '.' and '..' symbols
            if (strcmp(target_dir, ".") == 0) {
                // Current directory, no action needed
            } else if (strcmp(target_dir, "..") == 0) {
                // Move to parent directory
                chdir("..");
            } else {
                // Change to specified directory
                if (chdir(target_dir) != 0) {
                    perror("cd failed");
                }
            }
        }
		// Update cwd
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd() error");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(command, "echo") == 0) {
        // Echo command
        for (int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }
        printf("\n");
    } else if (strcmp(command, "export") == 0) {
        // Export command
        if (args[1] != NULL) {
            char* equal_sign = strchr(args[1], '=');
            if (equal_sign != NULL) {
                *equal_sign = '\0'; // Split at '='
                setenv(args[1], equal_sign + 1, 1);
            }
        }
    }
}

void execute_command(char* command, char** args, int background) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork() failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        // Execute built-in commands if applicable
        execute_shell_builtin(command, args);

        // Execute external commands
        if (execvp(command, args) == -1) {
            perror("execvp() failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        if (!background) {
            // Wait for child to finish if not executing in background
            waitpid(pid, NULL, 0);
        }
    }
}
