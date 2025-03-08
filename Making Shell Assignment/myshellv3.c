#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>  // Include this header for errno
#include <signal.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "PUCITshell:- "

int execute_pipeline(char** cmds[], int cmd_count, int background);
int execute_single(char* arglist[], int in_fd, int out_fd, int background);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);
void free_tokens(char** tokens);
void free_pipeline(char*** cmds, int cmd_count);
void handle_sigchld(int sig);

int main() {
   // Set up the signal handler to avoid zombie processes
   struct sigaction sa;
   sa.sa_handler = &handle_sigchld;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
   sigaction(SIGCHLD, &sa, NULL);

   char *cmdline;
   char** arglist;
   char* prompt = PROMPT;

   while ((cmdline = read_cmd(prompt, stdin)) != NULL) {
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

      execute_pipeline(parsed_cmds, cmd_count, background);

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

// Function to execute a pipeline of commands with optional background execution
int execute_pipeline(char** cmds[], int cmd_count, int background) {
   int fd[2], in_fd = 0;
   int status;

   for (int i = 0; i < cmd_count; i++) {
      pipe(fd);
      int pid = fork();
      if (pid == 0) {
         // Child process
         if (in_fd != 0) { // Not the first command
            dup2(in_fd, 0);
            close(in_fd);
         }
         if (i != cmd_count - 1) { // Not the last command
            dup2(fd[1], 1);
            close(fd[1]);
         }
         close(fd[0]); // Close unused read end of the pipe
         execvp(cmds[i][0], cmds[i]);
         perror("Command execution failed");
         exit(1);
      } else if (pid < 0) {
         perror("Fork failed");
         return -1;
      }
      // Parent process
      close(fd[1]); // Close unused write end of the pipe
      in_fd = fd[0]; // Set up input for the next command

      if (!background) {
         waitpid(pid, &status, 0); // Wait for the current process if not in background
      } else {
         printf("[%d] %d\n", i + 1, pid); // Display job number and PID
      }
   }

   // If running in the background, don't wait for the last command
   if (!background && cmd_count > 0) {
      waitpid(-1, &status, 0);
   }

   return 0;
}

// Function to execute a single command with optional I/O redirection and background execution
int execute_single(char* arglist[], int in_fd, int out_fd, int background) {
   int pid = fork();
   if (pid == -1) {
      perror("Fork failed");
      exit(1);
   } else if (pid == 0) {
      // Child process
      if (in_fd != 0) { // Redirect input
         dup2(in_fd, 0);
         close(in_fd);
      }
      if (out_fd != 1) { // Redirect output
         dup2(out_fd, 1);
         close(out_fd);
      }

      execvp(arglist[0], arglist);
      perror("Command not found");
      exit(1);
   } else {
      // Parent process
      if (!background) {
         int status;
         waitpid(pid, &status, 0);
      } else {
         printf("[Background PID] %d\n", pid); // Print background process PID
      }
      return 0;
   }
}

// Signal handler for SIGCHLD to clean up terminated background processes
void handle_sigchld(int sig) {
   int saved_errno = errno; // Save errno because waitpid() can change it
   while (waitpid(-1, NULL, WNOHANG) > 0);
   errno = saved_errno;
}

char** tokenize(char* cmdline) {
   // Allocate memory for argument list
   char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
   for (int i = 0; i < MAXARGS + 1; i++) {
      arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
      memset(arglist[i], 0, ARGLEN);
   }

   int argnum = 0; // Number of arguments
   char* token = strtok(cmdline, " ");

   while (token != NULL && argnum < MAXARGS) {
      strcpy(arglist[argnum], token);
      argnum++;
      token = strtok(NULL, " ");
   }
   arglist[argnum] = NULL; // NULL-terminate the argument list
   return arglist;
}

char* read_cmd(char* prompt, FILE* fp) {
   printf("%s", prompt);
   int c;
   int pos = 0;
   char* cmdline = (char*)malloc(sizeof(char) * MAX_LEN);

   while ((c = getc(fp)) != EOF) {
      if (c == '\n')
         break;
      cmdline[pos++] = c;
   }

   if (c == EOF && pos == 0) // Handle Ctrl+D
      return NULL;
   cmdline[pos] = '\0';
   return cmdline;
}

void free_tokens(char** tokens) {
   for (int i = 0; i < MAXARGS + 1; i++) {
      free(tokens[i]);
   }
   free(tokens);
}

void free_pipeline(char*** cmds, int cmd_count) {
   for (int i = 0; i < cmd_count; i++) {
      free_tokens(cmds[i]);
   }
}
