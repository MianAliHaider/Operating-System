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
    
    main():
        The shell prompt (PUCITshell:- ) is displayed, and user input is read via read_cmd().
        User input is split by the | character to handle pipelines.
        Each segment between the | is considered a separate command.
        The commands are tokenized (split into individual arguments) using tokenize().
        execute_pipeline() is called to execute the pipeline.
        Dynamically allocated memory for commands is freed after execution.
    
    execute_pipeline(): 
        Takes an array of commands (each command itself is an array of arguments) and the count of commands.
        Sets up a pipeline using pipes (pipe() system call) and forks a child process for each command.
        For the first command, it reads from standard input.
        For intermediate commands, it reads from the previous pipe's output and writes to the next pipe's input.
        For the last command, it reads from the last pipe but writes to standard output.
        Uses dup2() to redirect standard input/output as needed.
        Closes unused ends of pipes to prevent resource leaks.
        Waits for all child processes to complete with waitpid().
   
    execute_single():
        Executes a single command with optional I/O redirection.
        Handles input and output file descriptors.
        Uses fork() to create a child process, dup2() for redirection, and execvp() to execute the command.
    
    tokenize():
        Converts a command line string into an array of arguments.
        Uses strtok() to split the input string by spaces.
        Allocates memory for each argument and returns a NULL-terminated list of strings (arglist).
    
    read_cmd():
        Reads user input from the shell prompt.
        Handles end-of-file (Ctrl+D) to gracefully exit the shell.
        Stores the input in a dynamically allocated buffer and returns it.
    
    Memory Management:
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
       
        MAX_LEN: Maximum length for command input.
        MAXARGS: Maximum number of arguments a command can take.
        ARGLEN: Maximum length for each individual argument.
        PROMPT: The shell prompt displayed to the user.
        
        execute_pipeline: Executes a sequence of commands connected by pipes.
        execute_single: Executes a single command, optionally with input and output redirection.
        tokenize: Splits a command string into individual arguments.
        read_cmd: Reads a line of input from the user.
        free_tokens: Frees memory allocated for argument tokens.
        free_pipeline: Frees memory allocated for a pipeline of commands.
        handle_sigchld: Handles the SIGCHLD signal to reap zombie processes

        main(): The main function starts here. It sets up a signal handler for SIGCHLD to prevent zombie processes (processes that have finished execution but still 	have             an entry in the process table).
        
        	cmdline: Holds the user's command input.
        	arglist: To store parsed arguments for commands.
        	prompt: Holds the prompt string to be displayed.
        
        	A loop to continuously read commands from the user until EOF (Ctrl+D) is reached. It initializes flags and arrays to store commands.
        	Checks if the command is to be run in the background (indicated by a trailing &). If so, it removes the & from the command line.
        	Splits the command line into separate commands based on the pipe (|) character and stores them in the cmds array.
        	Each command in cmds is tokenized into arguments using the tokenize function, storing them in parsed_cmds.
        	Calls execute_pipeline to execute the commands in the pipeline.
        	Frees dynamically allocated memory for command arguments and the command line string to avoid memory leaks.
        	Ends the program with a newline and returns 0 to indicate successful completion.
        
        execute_pipeline(): 
        	Prepares to execute a pipeline of commands, setting up an array for file descriptors (fd), and initializing in_fd for input redirection.
        	Loops through each command to create pipes and fork a new process. If pid == 0, the current process is a child.
        	Redirects input from in_fd if it's not the first command and output to the pipe's write end (fd[1]) if it's not the last command. The unused read 	end (fd[0])              is closed. The command is executed using execvp. If execvp fails, an error is printed, and the child exits. If fork fails, an error is 	printed, and the                     function returns -1.
        	Closes the write end of the pipe in the parent process and sets up in_fd for the next command's input.
        	If not running in the background, waits for the current command to finish. If it is running in the background, it prints the job number and PID.
        	If not in background mode, waits for the last command in the pipeline to finish.
        
        execute_signal():
        	char* arglist[]: An array of strings representing the command and its arguments. The first element is the command to be executed, followed by any 	arguments.
        	int in_fd: A file descriptor for input redirection. If it’s not zero, it means input should be redirected from this descriptor.
        	int out_fd: A file descriptor for output redirection. If it’s not one, it means output should be redirected to this descriptor.
        	int background: A flag indicating whether the command should run in the background (1) or in the foreground (0).
        	The fork() function is called to create a new process (the child process). If fork() fails, it prints an error message and exits the program.
        	This block is executed by the child process (where pid is 0).
        	If in_fd is not 0, it indicates that input should be redirected. The dup2() function duplicates the file descriptor in_fd to standard input (0), 	effectively              redirecting the input. After this, close(in_fd) is called to close the original file descriptor.
        	Similarly, if out_fd is not 1, the output is redirected to the specified file descriptor. dup2() duplicates out_fd to standard output (1), and the 	original                 file descriptor is closed afterward.
        	The execvp() function attempts to execute the command specified by arglist[0] with the arguments provided in arglist. If the command fails to 	execute (for                 example, if the command is not found), it prints an error message and exits the child process.
        	This block is executed by the parent process (where pid is greater than 0).
        	If the background flag is 0, the parent process waits for the child process to finish executing using waitpid(). This blocks the parent until the 	child                     terminates.
        	If the background flag is 1, the parent does not wait for the child to finish; instead, it prints the child's process ID (PID) to indicate that it 	is running               in the background.
        
        handle_sigchld():
        	int saved_errno = errno;: Saves the current value of errno to preserve its state because waitpid() can modify it.
        	while (waitpid(-1, NULL, WNOHANG) > 0);: Calls waitpid() in a loop to reap any terminated child processes without blocking. The -1 argument makes it 	wait for             any child process. WNOHANG makes it return immediately if no child has exited.
        	errno = saved_errno: Restores errno to its original value after cleaning up the child processes.
        tokenize():
        	This function splits a command line into individual arguments.
        	Allocates an array of pointers (arglist) to hold each argument. The size is MAXARGS + 1 to accommodate the null terminator.
        	A loop allocates memory for each argument and initializes it with zero using memset.
        	The function uses strtok to split the cmdline string into tokens based on spaces.
        	It copies each token into the arglist until MAXARGS is reached or there are no more tokens.
        	arglist[argnum] = NULL;: Terminates the list with a NULL pointer to signify the end of arguments.
        read_cmd():
        	This function reads a line of input from the user, displaying a prompt.
        	Displays the prompt using printf.
        	Initializes variables for reading characters and storing the command line.
        	The function reads characters one by one until a newline (\n) or EOF is encountered.
        	If EOF is reached and no characters have been read (pos == 0), it returns NULL (indicating end-of-file, such as when the user presses Ctrl+D).
        	The command line is null-terminated before returning it.
        free_tokens():
        	This function frees the memory allocated for an array of argument tokens.
        	Iterates through the tokens array and frees each individual argument string.
        	Finally, it frees the array of pointers itself.
        free_pipeline():
        	This function frees memory allocated for a pipeline of commands.
        	Iterates through each command in the cmds array and calls free_tokens to clean up memory for the tokens associated with that command.
        
        
        
        
