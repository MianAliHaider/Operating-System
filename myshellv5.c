#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h> // Include readline history functionality

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "PUCITshell:- "
#define HISTORY_SIZE 10

// Function prototypes
int execute_pipeline(char** cmds[], int cmd_count, int background);
char** tokenize(char* cmdline);
char* read_cmd(char* prompt);
void free_tokens(char** tokens);
void add_to_history(const char* cmd);
char* get_history_command(int index);
void execute_builtin(char** args);

// Command history array
char* command_history[HISTORY_SIZE];
int history_count = 0;

// Background process list
pid_t background_processes[MAX_LEN];
int bg_process_count = 0;

// Signal handler for SIGCHLD to clean up terminated background processes
void handle_sigchld(int sig) {
    int saved_errno = errno; // Save errno because waitpid() can change it
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// Main function
int main() {
    // Set up the signal handler to avoid zombie processes
    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    char* cmdline;
    char* prompt = PROMPT;

    // Initialize command history
    memset(command_history, 0, sizeof(command_history));
    memset(background_processes, 0, sizeof(background_processes));

    while ((cmdline = read_cmd(prompt)) != NULL) {
        // Check for history expansion
        if (cmdline[0] == '!') {
            int history_index = atoi(&cmdline[1]) - 1; // Convert to zero-based index
            if (history_index >= 0 && history_index < history_count) {
                // Get the command from history
                free(cmdline); // Free the current command line
                cmdline = strdup(command_history[history_index]); // Get the history command
                printf("Repeating command: %s\n", cmdline); // Show the command being repeated
            } else {
                printf("No such command in history\n");
                free(cmdline);
                continue; // Skip the rest of the loop
            }
        } else {
            // Add the command to history if it's not empty
            if (strlen(cmdline) > 0) {
                add_to_history(cmdline);
            }
        }

        int background = 0;
        char* cmds[MAXARGS];
        int cmd_count = 0;

        // Check if the command should be run in the background
        if (cmdline[strlen(cmdline) - 1] == '&') {
            background = 1;
            cmdline[strlen(cmdline) - 1] = '\0'; // Remove '&' from the end
        }

        // Split input by pipes
        char* token = strtok(cmdline, "|");
        while (token != NULL && cmd_count < MAXARGS) {
            cmds[cmd_count++] = strdup(token);
            token = strtok(NULL, "|");
        }

        // Parse each command in the pipeline and execute
        char** parsed_cmds[MAXARGS] = {NULL};
        for (int i = 0; i < cmd_count; i++) {
            parsed_cmds[i] = tokenize(cmds[i]);
        }

        // Execute built-in commands or pipeline
        if (strcmp(parsed_cmds[0][0], "exit") == 0) {
            exit(0);
        } else if (strcmp(parsed_cmds[0][0], "help") == 0) {
            printf("Available commands:\n");
            printf("cd <dir>   - Change the working directory to <dir>\n");
            printf("exit       - Exit the shell\n");
            printf("jobs       - List background jobs\n");
            printf("kill <pid> - Terminate the process with the specified <pid>\n");
            printf("help       - Display this help message\n");
        } else if (strcmp(parsed_cmds[0][0], "jobs") == 0) {
            printf("Background jobs:\n");
            for (int i = 0; i < bg_process_count; i++) {
                printf("[%d] %d\n", i + 1, background_processes[i]);
            }
        } else if (strcmp(parsed_cmds[0][0], "cd") == 0) {
            if (parsed_cmds[0][1] != NULL) {
                if (chdir(parsed_cmds[0][1]) != 0) {
                    perror("cd failed");
                }
            } else {
                fprintf(stderr, "cd: missing argument\n");
            }
        } else if (strcmp(parsed_cmds[0][0], "kill") == 0) {
            if (parsed_cmds[0][1] != NULL) {
                pid_t pid = atoi(parsed_cmds[0][1]);
                if (kill(pid, SIGKILL) == -1) {
                    perror("kill failed");
                }
            } else {
                fprintf(stderr, "kill: missing argument\n");
            }
        } else {
            execute_pipeline(parsed_cmds, cmd_count, background);
        }

        // Free dynamically allocated memory
        for (int i = 0; i < cmd_count; i++) {
            free_tokens(parsed_cmds[i]);
            free(cmds[i]);
        }
        free(cmdline);
    }
    printf("\n");
    return 0;
}

// Function to execute a pipeline of commands
int execute_pipeline(char** cmds[], int cmd_count, int background) {
    int fd[2], in_fd = 0;
    int status;

    for (int i = 0; i < cmd_count; i++) {
        // Create pipe for the current command
        if (i < cmd_count - 1) {
            pipe(fd);
        }

        char** arglist = cmds[i]; // Get the current command arguments

        // Handle output redirection
        int out_fd = STDOUT_FILENO; // Default to write end of pipe
        for (int j = 0; arglist[j] != NULL; j++) {
            if (strcmp(arglist[j], ">") == 0) {
                if (arglist[j + 1] != NULL) { // Check for filename
                    out_fd = open(arglist[j + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    if (out_fd < 0) {
                        perror("Failed to open output file");
                        return -1;
                    }
                    arglist[j] = NULL; // Remove the '>' and filename from arglist
                }
            } else if (strcmp(arglist[j], "<") == 0) {
                if (arglist[j + 1] != NULL) { // Check for input file
                    in_fd = open(arglist[j + 1], O_RDONLY);
                    if (in_fd < 0) {
                        perror("Failed to open input file");
                        return -1;
                    }
                    arglist[j] = NULL; // Remove '<' and filename from arglist
                }
            }
        }

        int pid = fork();
        if (pid == 0) {
            // Child process
            if (in_fd != STDIN_FILENO) { // Redirect input if necessary
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < cmd_count - 1) { // Redirect output to the write end of the pipe
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]); // Close unused read end
            } else if (out_fd != STDOUT_FILENO) { // Handle output redirection
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            execvp(arglist[0], arglist);
            perror("Command execution failed");
            exit(1);
        } else if (pid < 0) {
            perror("Fork failed");
            return -1;
        }
        // Parent process
        if (in_fd != STDIN_FILENO) {
            close(in_fd); // Close the input file descriptor in parent
        }
        if (i < cmd_count - 1) {
            close(fd[1]); // Close the write end of the pipe
        }
        in_fd = fd[0]; // Set input for the next command

        if (!background) {
            waitpid(pid, &status, 0); // Wait for the current process if not in background
        } else {
            background_processes[bg_process_count++] = pid; // Store the background process
            printf("[%d] %d\n", pid, bg_process_count); // Display job number and PID
        }
    }

    // If running in the background, don't wait for the last command
    if (!background && cmd_count > 0) {
        waitpid(-1, &status, 0);
    }

    return 0;
}

// Tokenize the command line into arguments
char** tokenize(char* cmdline) {
    char** tokens = malloc(MAXARGS * sizeof(char*));
    char* token = strtok(cmdline, " \n");
    int i = 0;

    while (token != NULL && i < MAXARGS - 1) {
        tokens[i++] = strdup(token);
        token = strtok(NULL, " \n");
    }
    tokens[i] = NULL; // Null-terminate the array
    return tokens;
}

// Read a command line with a prompt
char* read_cmd(char* prompt) {
    char* input = readline(prompt);
    return input; // Return input
}

// Free allocated tokens
void free_tokens(char** tokens) {
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

// Add command to history
void add_to_history(const char* cmd) {
    if (history_count < HISTORY_SIZE) {
        command_history[history_count++] = strdup(cmd);
    } else {
        free(command_history[0]); // Remove the oldest command
        for (int i = 1; i < HISTORY_SIZE; i++) {
            command_history[i - 1] = command_history[i]; // Shift commands left
        }
        command_history[HISTORY_SIZE - 1] = strdup(cmd); // Add new command at the end
    }
}

// Get a command from history
char* get_history_command(int index) {
    if (index >= 0 && index < history_count) {
        return command_history[index];
    }
    return NULL; // Return NULL if index is out of bounds
}
