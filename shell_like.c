#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define N 256
#define SIZE 10
#define HISTORY_LIMIT 10

typedef struct node // linked list structure
{
    char *command;
    struct node *next;
} node;

node *head = NULL; // initialize global head pointer for linked list

void print_directory() // "dir" function
{
    char current_directory[N];
    if (getcwd(current_directory, sizeof(current_directory)) == NULL)
    {
        perror("getcwd error\n");
    }
    else
    {
        printf("%s\n", current_directory);
    }
}

void change_directory(char *user_input) // "cd" function
{
    if (strlen(user_input) == 2)
    {
        chdir(getenv("HOME"));
        setenv(getenv("HOME"), "HOME", 1);
    }
    else
    {
        chdir(user_input + 3);
        setenv(getenv(user_input + 3), "PWD", 1);
    }
}

void delete_fgets_newline(char *user_input) // fgets() method reads a string and ends the string with a new line '\n', I wanted to replace this with '\0'
{
    if ((strlen(user_input) > 0) && (user_input[strlen(user_input) - 1] == '\n'))
    {
        user_input[strlen(user_input) - 1] = '\0';
    }
}

void parse_without_pipe(char *user_input, char **arguments, int *j) // user input will be parsed with this function if no '|' character is present in the user input
{
    char *token = strtok(user_input, " "); // get the first token
    *(arguments + *j) = token;

    while (token != NULL) // walk through tokens (which is space in this case)
    {
        *j += 1;
        token = strtok(NULL, " ");
        *(arguments + *j) = token;
    }
}

void print_history(node *head) // function to print linked lists which are storing the command history of the user
{
    node *current = head;
    int i = 1;
    while (current != NULL)
    {
        printf("[%d] %s\n", i, (current->command));
        current = current->next;
        i++;
    }
}

void add_command_to_history(char *user_input, node **head) // whenever the user enters a new command, the command will be added to the linked list
{
    node *new_node;
    new_node = malloc(sizeof(node)); // gives address to new node
    // new_node->command = user_input;
    new_node->command = malloc(sizeof(char *));
    strcpy(new_node->command, user_input);
    new_node->next = NULL;

    if (*head == NULL) // create head for linked list
    {
        *head = new_node;
    }

    else
    {
        int count = 1;
        node *current = *head;
        while (current->next != NULL) // propogate among the nodes
        {
            current = current->next;
            count++;
        }
        current->next = new_node; // add new node to the end
        if (count == HISTORY_LIMIT) // remove the last node so that it does not exceed HISTORY_LIMIT = 10
        {
            node *temp = *head;
            *head = (*head)->next;
            free(temp);
        }
    }
}

int main(int argc, char *argv)
{
    while (1)
    {
        int j = 0;
        char user_input[N] = "";
        char *arguments[SIZE];

        printf("myshell>");
        fgets(user_input, N, stdin); // get the input from the user

        delete_fgets_newline(user_input); // fgets includes a new line at the end, this function will remove it

        if (user_input[0] == '\0') // do nothing if user presses enter
        {
            continue;
        }

        add_command_to_history(user_input, &head); // add the user input into the history

        if (strcmp(user_input, "dir") == 0) // check whether user input is "dir"
        {
            print_directory();
        }
        else if (strcmp(user_input, "bye") == 0) // check whether user input is "bye"
        {
            exit(0);
        }
        else if (strcmp(user_input, "history") == 0) // check whether user input is "history"
        {
            print_history(head);
        }
        else if (((strlen(user_input) == 2) && (strncmp(user_input, "cd", 2)) == 0) || ((strlen(user_input) > 3) && (strncmp(user_input, "cd ", 3)) == 0)) // checking whether the input is solely "cd" and of length 2 or starts with characters "cd " and of length 3 which are intended for changing directory
        {
            change_directory(user_input);
        }

        else
        {
            if (strchr(user_input, '|') == NULL) // condition where the user input does not include character '|'
            {
                parse_without_pipe(user_input, arguments, &j); // parse the input

                if (strcmp(arguments[j - 1], "&") == 0) // if the user enters character '&' at the end of the command
                {
                    arguments[j - 1] = NULL; // removing '&' at the end
                    pid_t pid_1 = fork();
                    if (pid_1 < 0)
                    {
                        perror("fork error");
                    }
                    else if (pid_1 == 0) // child
                    {
                        execvp(arguments[0], arguments);
                        exit(0);
                    }
                    else if (pid_1 > 0) // parent
                    {
                    }
                }
                else // user input without & at the end
                {
                    pid_t pid_1 = fork();

                    if (pid_1 < 0)
                    {
                        perror("fork error");
                    }
                    else if (pid_1 == 0) // child
                    {
                        execvp(arguments[0], arguments);
                        exit(0);
                    }
                    else if (pid_1 > 0) // parent
                    {
                        wait(NULL);
                    }
                }
            }
            else // condition where the user input includes character '|'
            {
                int k = 0;
                int l = 0;
                char *ptr = user_input;
                char *before_pipe;
                char *after_pipe;
                char *arguments_before_pipe[N];
                char *arguments_after_pipe[N];

                before_pipe = strsep(&ptr, "|"); // extracting the left side of pipe
                before_pipe[strlen(before_pipe) - 1] = '\0'; // removing the new line at the end of the extracted left side
                after_pipe = ptr + 1; // extracting right side of the pipe

                parse_without_pipe(before_pipe, arguments_before_pipe, &k); // character '|' has been dealt with, so we parse the left of pipe here
                parse_without_pipe(after_pipe, arguments_after_pipe, &l); // parsing the right side of pipe

                if (strcmp(arguments_after_pipe[l - 1], "&") == 0) // condition where the user input ends with '&'
                {
                    arguments_after_pipe[l - 1] = NULL; // removing '&' at the end of the user input
                    int pipefd[2]; // set up pipes
                    pid_t p1, p2;

                    pipe(pipefd);

                    p1 = fork(); // fork the first child that will write inside the write end of the pipe

                    if (p1 < 0)
                    {
                        printf("fork error\n");
                        return 1;
                    }

                    else if (p1 == 0) // first child
                    {
                        close(pipefd[0]);
                        dup2(pipefd[1], STDOUT_FILENO);
                        close(pipefd[1]);
                        execvp(arguments_before_pipe[0], arguments_before_pipe); // execute the left part of the pipe
                    }

                    else // parent
                    {
                        p2 = fork(); // fork the second child that will read the pipe

                        if (p2 < 0)
                        {
                            printf("fork error\n");
                            return 1;
                        }

                        else if (p2 == 0) // second child
                        {
                            close(pipefd[1]);
                            dup2(pipefd[0], STDIN_FILENO);
                            close(pipefd[0]);
                            execvp(arguments_after_pipe[0], arguments_after_pipe); // execute the right part of the pipe
                        }

                        else // parent
                        {
                            close(pipefd[1]);
                            close(pipefd[0]);
                        }
                    }
                }
                else // condition where the user input does not end with '&'
                {

                    int pipefd[2]; // create pipe
                    pid_t p1, p2;

                    pipe(pipefd);

                    p1 = fork(); // fork the first child that will write inside the write end of the pipe

                    if (p1 < 0)
                    {
                        printf("fork error\n");
                        return 1;
                    }

                    else if (p1 == 0) // first child
                    {
                        close(pipefd[0]);
                        dup2(pipefd[1], STDOUT_FILENO);
                        close(pipefd[1]);
                        execvp(arguments_before_pipe[0], arguments_before_pipe); // execute the left part of the pipe
                        exit(0);
                    }

                    else // parent
                    {
                        p2 = fork(); // fork the second child that will read the pipe

                        if (p2 < 0)
                        {
                            printf("fork error\n");
                            return 1;
                        }

                        else if (p2 == 0) // second child
                        {
                            close(pipefd[1]);
                            dup2(pipefd[0], STDIN_FILENO);
                            close(pipefd[0]);
                            execvp(arguments_after_pipe[0], arguments_after_pipe); // execute the right part of the pipe
                            exit(0);
                        }

                        else // parent
                        {
                            close(pipefd[1]);
                            close(pipefd[0]);
                            wait(NULL); // wait for both children
                            wait(NULL);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
