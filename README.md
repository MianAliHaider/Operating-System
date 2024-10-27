# Operating-System
**Version01 (myshellv1.c):**
    main function():
        Display the prompt(PUCITshell:-)
        Reads a command from the user input using read_cmd()
        Tokenizes the command using tokenize().
        Execute the command using execute().
    read_cmd():
        Display the shell prompt.
        Reads the user input until the newline or EOF is encountered.
        Handles the case where the user presses the <ctrl+D> to exit the shell by returning null.
    tokenize():
        Takes the user command string and splits it into individual tolens.
        Allocates memory for a list () where each slot can store a word.
        Returns an array of strings which is passed to the execute function.
    execute():
        fork() a new processes using fork().
        In the child process, it usus execvp() to run the command provided by the arglist. If the command is not found it display an error message.
        In the parent process, it waits for the child process to finish using waitpid() and then reports the exit status.
        fork(): Creates a child process. It returns:
              -1 if the fork failed.
              0 to the child process.
              A positive process ID (PID) to the parent process.
              execvp(char* file, char* argv[]): Replaces the current process image with a new process image (executes a command). It uses:
              file: The command to execute.
              argv[]: The argument list (array of strings).
              waitpid(pid_t pid, int* status, int options): Makes the parent process wait until the child process with the given pid terminates.

**version02(myshellv2.c):**
    Single Command Execution: Supports execution of single commands like ls, echo hello, etc.
    Pipeline Support: Allows executing multiple commands connected by a pipe, like ls | grep ".c" | sort.
    Memory Management: Handles dynamic memory allocation and cleanup for commands and their arguments. 
    main() Function
        The shell prompt (PUCITshell:- ) is displayed, and user input is read via read_cmd().
        User input is split by the | character to handle pipelines.
        Each segment between the | is considered a separate command.
        The commands are tokenized (split into individual arguments) using tokenize().
        execute_pipeline() is called to execute the pipeline.
        Dynamically allocated memory for commands is freed after execution.

    execute_pipeline() Function
        Takes an array of commands (each command itself is an array of arguments) and the count of commands.
        Sets up a pipeline using pipes (pipe() system call) and forks a child process for each command.
        For the first command, it reads from standard input.
        For intermediate commands, it reads from the previous pipe's output and writes to the next pipe's input.
        For the last command, it reads from the last pipe but writes to standard output.
        Uses dup2() to redirect standard input/output as needed.
        Closes unused ends of pipes to prevent resource leaks.
        Waits for all child processes to complete with waitpid().
            
    execute_single() Function
        Executes a single command with optional I/O redirection.
        Handles input and output file descriptors.
        Uses fork() to create a child process, dup2() for redirection, and execvp() to execute the command.

    tokenize() Function
        Converts a command line string into an array of arguments.
        Uses strtok() to split the input string by spaces.
        Allocates memory for each argument and returns a NULL-terminated list of strings (arglist).
        
    read_cmd() Function
        Reads user input from the shell prompt.
        Handles end-of-file (Ctrl+D) to gracefully exit the shell.
        Stores the input in a dynamically allocated buffer and returns it.
        
    Memory Management Functions
        free_tokens(): Frees memory allocated for tokenized command arguments.
        free_pipeline(): Frees memory for the entire pipeline of commands.
        pipe(int fd[2]): Creates a unidirectional data channel (pipe). fd[0] is for reading, and fd[1] is for writing.
        fork(): Creates a new child process. If the value is:
                0: Child process.
                Positive integer: Parent process.
                -1: Fork failed
        dup2(int oldfd, int newfd): Duplicates file descriptors, used to redirect standard input/output.
        execvp(char* file, char* argv[]): Replaces the current process with a new process (command). The command is found using the system's PATH environment variable.
        waitpid(pid_t pid, int* status, int options): Waits for a specific child process to terminate.

**version03(myshellv3.c):**


        
