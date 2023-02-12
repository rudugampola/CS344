#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#define MAXLENGTH 2048
#define MAXARGS 512
#define PROCESS_LIMIT 100

#define BOLD_BLACK "\033[1;30m"
#define BOLD_RED "\033[1;91m"
#define BOLD_YELLOW "\033[0;93m"
#define BOLD_BLUE "\033[1;34m"
#define BOLD_PURPLE "\033[1;35m"
#define BOLD_CYAN "\033[1;36m"
#define BOLD_WHITE "\033[1;37m"
#define BOLD_GREEN "\033[1;92m"
#define NC "\033[0m"

// Use this struct to store the arguments, input file, output file, and background mode
struct command
{
    char *args[MAXARGS];
    char input_file[MAXARGS];
    char output_file[MAXARGS];
    int background_mode;
};

char input[MAXLENGTH] = {'\0'};
struct command command;
int childStatus;                 // Used to store the status of the child process
int exit_status = 0;             // default exit status of last foreground command
int processArray[PROCESS_LIMIT]; // Store process PIDs
int proccesIter;                 // Keep track of where to place PID

int command_prompt()
{
    struct command firstCommand = {.args = {'\0'}, .background_mode = 0, .input_file = {'\0'}, .output_file = {'\0'}}; // Initialize blank command struct
    command = firstCommand;                                                                                            // Set global command struct to this blank command struct

    fflush(stdout);                             // Flush stdout buffer
    if (fgets(input, MAXLENGTH, stdin) == NULL) // Handle EOF
    {
        if (feof(stdin))
        {
            return 0;
        }
        else
        {
            perror("fgets");
            exit(1);
        }
    }
    if (input[0] == '#') // If the first character is a #, ignore the line
    {
        return 2;
    }
    if (input[0] == '\n') // If the first character is a newline, ignore the line
    {
        return 2;
    }
    if (input[0] == ' ') // If the first character is a null terminator, ignore the line
    {
        return 2;
    }
    else
    {
        input[strlen(input) - 1] = '\0';
    }

    return 1;
}

// This function handles the input file
static int handle_input_file(char **token)
{
    *token = strtok(NULL, getenv("IFS"));
    if (*token == NULL)
        return 0;
    strcpy(command.input_file, *token);
    return 2;
}

// This function handles the output file
static int handle_output_file(char **token)
{
    *token = strtok(NULL, getenv("IFS"));
    if (*token == NULL)
        return 0;
    strcpy(command.output_file, *token);
    return 2;
}

static void variable_expansion(char *token)
{
    char *home = getenv("HOME");
    if (home != NULL && token[0] == '~' && token[1] == '/')
    {
        int home_len = strlen(home);
        int token_len = strlen(token);
        int new_len = home_len + token_len - 1; // -1 to remove the ~ from the token
        char *temp_tok = malloc(new_len + 1);   // +1 for null terminator
        if (!temp_tok)
        { // malloc error check
            perror("malloc");
            exit(1);
        }
        strcpy(temp_tok, home);      // copy home to temp_tok
        strcat(temp_tok, token + 1); // copy token to temp_tok, starting at index 1 to skip the ~
        strcpy(token, temp_tok);
        free(temp_tok);
    }

    int pid = getpid();
    char pid_str[6];
    sprintf(pid_str, "%d", pid);
    char *p = strstr(token, "$$");
    while (p != NULL)
    {
        int len = strlen(token) + strlen(pid_str) - 2;
        char temp_tok[len + 1];
        strncpy(temp_tok, token, p - token);
        strcat(temp_tok, pid_str);
        strcat(temp_tok, p + 2);
        strcpy(token, temp_tok);
        p = strstr(token, "$$");
    }

    char exit_status_str[6];
    sprintf(exit_status_str, "%d", exit_status);
    char *q = strstr(token, "$?");
    while (q != NULL)
    {
        int len = strlen(token) + strlen(exit_status_str) - 2;
        char temp_tok[len + 1];
        strncpy(temp_tok, token, q - token);
        strcat(temp_tok, exit_status_str);
        strcat(temp_tok, q + 2);
        strcpy(token, temp_tok);
        q = strstr(token, "$?");
    }

    char bg_pid_str[10] = ""; // default background process ID
    if (proccesIter > 0)
    {
        sprintf(bg_pid_str, "%d", processArray[proccesIter - 1]);
    } // if there is a background process, set the background process ID to the last process in the array

    char *r = strstr(token, "$!");
    while (r != NULL)
    {
        int len = strlen(token) + strlen(bg_pid_str) - 2;
        char temp_tok[len + 1];
        strncpy(temp_tok, token, r - token);
        strcat(temp_tok, bg_pid_str);
        strcat(temp_tok, r + 2);
        strcpy(token, temp_tok);
        r = strstr(token, "$!");
    }

    // Add null terminator to end of token
    token[strlen(token)] = '\0';
}

int tokenize_input()
{
    int count = -1;
    char *token = strtok(input, getenv("IFS"));

    while (token != NULL)
    {
        if (token[0] == '#')
            break; // If the first character is a #, ignore the rest of the line

        if (strcmp(token, "<") == 0)
        {
            count -= handle_input_file(&token);
        }
        else if (strcmp(token, ">") == 0)
        {
            count -= handle_output_file(&token);
        }
        else
        {
            char token_copy[MAXLENGTH];
            strcpy(token_copy, token);
            variable_expansion(token_copy);
            command.args[++count] = strdup(token_copy);
        }

        token = strtok(NULL, getenv("IFS"));
    }

    if (count >= 0 && strcmp(command.args[count], "&") == 0)
    {
        command.background_mode = 1;
        command.args[count] = NULL;
        count--;
    }

    return count; // Return the number of arguments in the command struct
}

void execute_non_builtin()
{
    // Fork a new process
    pid_t spawnPid = fork();

    switch (spawnPid)
    {
    case -1:
        perror("fork() failed!\n");
        fflush(stdout);
        exit(1);
        break;
    case 0:
        if (command.input_file[0] != '\0')
        {
            int input_fd = open(command.input_file, O_RDONLY);
            if (input_fd == -1)
            {
                perror("open() failed!\n");
                fflush(stdout);
                exit(1);
            }

            dup2(input_fd, 0);                    // Redirect stdin to input file
            close(input_fd);                      // Close the file descriptor
            fcntl(input_fd, F_SETFD, FD_CLOEXEC); // Close the file descriptor on exec
        }
        if (command.output_file[0] != '\0')
        {
            int output_fd = open(command.output_file, O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (output_fd == -1)
            {
                perror("open() failed!\n");
                fflush(stdout);
                exit(1);
            }

            dup2(output_fd, 1); // Redirect stdout to output file
            close(output_fd);   // Close the file descriptor

            fcntl(output_fd, F_SETFD, FD_CLOEXEC); // Close the file descriptor on exec
        }

        if (execvp(command.args[0], command.args) == -1) // If execvp() returns, it must have failed
        {
            perror("execvp() failed!\n"); // If execvp() returns, it must have failed
            fflush(stdout);
            exit(EXIT_FAILURE);
        }

    default:
        // In the parent process
        if (command.background_mode == 1)
        {
            processArray[proccesIter] = spawnPid; // Add child process to process array.
            proccesIter += 1;
            waitpid(spawnPid, &childStatus, WUNTRACED | WNOHANG); // Dont wait.
            exit_status = WEXITSTATUS(childStatus);

            fflush(stdout);
        }
        else
        {
            spawnPid = waitpid(spawnPid, &childStatus, WUNTRACED | WCONTINUED);
            if (spawnPid == -1)
            {
                perror("waitpid() failed!\n");
                fflush(stdout);
                exit(EXIT_FAILURE);
            }
            if (WIFEXITED(childStatus))
            {
                exit_status = WEXITSTATUS(childStatus);
            }

            else
            {
                exit_status = WTERMSIG(childStatus);
            }
        }

        break;
    }
}

void execute_command()
{
    /*
    Built-in commands
    */
    if (strcmp(command.args[0], "exit") == 0)
    {
        // If more than 1 argument, print error to stderr
        if (command.args[2] != NULL || isdigit(command.args[1][0]) == 0)
        {
            printf(BOLD_RED "Error: exit command does not take any arguments\n" NC);
            return;
        }
        else
        {
            exit(atoi(command.args[1]));
            exit_status = 0;
        }
    }

    else if (strcmp(command.args[0], "cd") == 0)
    {
        // If more than 1 argument, print error to stderr
        if (command.args[2] != NULL)
        {
            fprintf(stderr, BOLD_RED "Error: cd command does not take more than one argument\n" NC);
            return;
        }
        else
        {
            char *new_dir = malloc(256 * sizeof(char)); // string for holding name of new directory
            strcpy(new_dir, getenv("HOME"));            // set new directory to HOME PATH
            if (new_dir == NULL)
            {
                fprintf(stderr, BOLD_RED "Error: unable to get HOME PATH.\n" NC);
                return;
            }

            if (command.args[1] == NULL)
            {
                int chdir_error;
                chdir(new_dir); // if no argument, change to HOME PATH
                chdir_error = chdir(new_dir);
                if (chdir_error == -1)
                {
                    fprintf(stderr, BOLD_RED "Error: unable to change directory to %s\n" NC, new_dir);
                    fflush(stdout);
                }
            }
            else
            {
                strcat(new_dir, "/");
                strcat(new_dir, command.args[1]); // if argument, change to HOME PATH + argument
                int chdir_error;
                chdir_error = chdir(new_dir);
                if (chdir_error == -1)
                {
                    fprintf(stderr, BOLD_RED "Error: unable to change directory to %s\n" NC, new_dir);
                    fflush(stdout);
                }
            }

            free(new_dir);
            return;
        }
    }

    /*
    Non-Builtin Commands
    */
    else
    {
        execute_non_builtin();
    }
}

// Function to check for background processes and print their status
void background_processes()
{
    for (int i = 0; i < proccesIter; i++)
    { // Go through our background processes
        if (waitpid(processArray[i], &childStatus, WNOHANG | WUNTRACED) > 0)
        {
            if (WIFSTOPPED(childStatus))
            {
                printf(BOLD_GREEN "Child process %d stopped. Continuing.\n" NC, processArray[i]); // Child PID and signal
                fflush(stdout);
            }

            if (WIFSIGNALED(childStatus))
            { // check if child terminated with signal

                printf(BOLD_GREEN "Child process %d done. Signaled %d.\n" NC, processArray[i], WTERMSIG(childStatus)); // Child PID and signal
                fflush(stdout);
            }

            if (WIFEXITED(childStatus))
            { // Check if child process terminated normally

                printf(BOLD_GREEN "Child process %d done. Exit status %d.\n" NC, processArray[i], WEXITSTATUS(childStatus)); // Child PID and exit status
                fflush(stdout);
            }
        }
    }
}

int main()
{
    // set IFS to space, tab, and newline
    if (getenv("IFS") == NULL)
    {
        setenv("IFS", " \t\n", 1);
    }

    while (1)
    {
        if (command_prompt() == 1)
        {
            tokenize_input();
            execute_command();
        }
        background_processes();
    }
}