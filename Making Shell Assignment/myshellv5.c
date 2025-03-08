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
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "PUCITshell:- "
#define HISTORY_SIZE 100

// Function prototypes
int execute_pipeline(char** cmds[], int cmd_count, int background);
char** tokenize(char* cmdline);
char* read_cmd(char* prompt);
void free_tokens(char** tokens);
void add_to_history(const char* cmd);
void execute_history_command(int index);

pid_t background_processes[MAX_LEN];
int bg_process_count = 0;

// Signal handler for SIGCHLD to clean up terminated background processes
void handle_sigchld(int sig) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int main() {
    // Set up the signal handler to avoid zombie processes
    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    using_history();  // Initialize history handling
    char* cmdline;

    while ((cmdline = read_cmd(PROMPT)) != NULL) {
        // Check for history command
        if (cmdline[0] == '!') {
            int index = atoi(&cmdline[1]);
            if (index > 0 && index <= history_length) {
                execute_history_command(index);
                free(cmdline);
                continue;
            } else {
                fprintf(stderr, "No such command in history: %d\n", index);
                free(cmdline);
                continue;
            }
        }

        // Add command to history
        if (strlen(cmdline) > 0) {
            add_history(cmdline);  // Add the command to history for arrow key navigation
        }

        // Check if the command should be run in the background
        int background = 0;
        if (cmdline[strlen(cmdline) - 1] == '&') {
            background = 1;
            cmdline[strlen(cmdline) - 1] = '\0'; // Remove '&' from the end
        }

        int cmd_count = 0;
        char* cmds[MAXARGS];

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

        char** arglist = cmds[i];

        // Handle output redirection
        int out_fd = STDOUT_FILENO;
        for (int j = 0; arglist[j] != NULL; j++) {
            if (strcmp(arglist[j], ">") == 0) {
                if (arglist[j + 1] != NULL) {
                    out_fd = open(arglist[j + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    if (out_fd < 0) {
                        perror("Failed to open output file");
                        return -1;
                    }
                    arglist[j] = NULL;
                }
            } else if (strcmp(arglist[j], "<") == 0) {
                if (arglist[j + 1] != NULL) {
                    in_fd = open(arglist[j + 1], O_RDONLY);
                    if (in_fd < 0) {
                        perror("Failed to open input file");
                        return -1;
                    }
                    arglist[j] = NULL;
                }
            }
        }

        int pid = fork();
        if (pid == 0) {
            // Child process
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < cmd_count - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
            } else if (out_fd != STDOUT_FILENO) {
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
        
        if (in_fd != STDIN_FILENO) {
            close(in_fd);
        }
        if (i < cmd_count - 1) {
            close(fd[1]);
        }
        in_fd = fd[0];

        if (!background) {
            waitpid(pid, &status, 0);
        } else {
            background_processes[bg_process_count++] = pid;
            printf("[%d] %d\n", bg_process_count, pid);  // Print background job id
        }
    }

    if (!background && cmd_count > 0) {
        waitpid(-1, &status, 0);
    }

    return 0;
}

// Tokenize the command line into arguments
char** tokenize(char* cmdline) {
    char** tokens = malloc(MAXARGS * sizeof(char*));
    if (!tokens) {
        perror("Failed to allocate memory for tokens");
        exit(1);
    }
    char* token = strtok(cmdline, " \n");
    int i = 0;

    while (token != NULL && i < MAXARGS - 1) {
        tokens[i++] = strdup(token);
        token = strtok(NULL, " \n");
    }
    tokens[i] = NULL;
    return tokens;
}

// Read a command line with a prompt
char* read_cmd(char* prompt) {
    char* input = readline(prompt);
    return input;
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
    if (strlen(cmd) > 0) {
        add_history(cmd);
    }
}

// Execute command from history
void execute_history_command(int index) {
    HIST_ENTRY *entry = history_get(index);
    if (entry) {
        char *cmdline = strdup(entry->line);
        printf("Executing: %s\n", cmdline);

        // Call the main command execution logic with the retrieved command line
        // Instead of main_loop, you can directly invoke the execution logic here.
        // Implement the logic similar to the one in main() for processing the command.
        // For simplicity, we're just printing the command here.
        free(cmdline);
    }
}
