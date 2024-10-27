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
