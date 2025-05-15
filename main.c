#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char *lsh_read_line(void);
char **lsh_split_line(char *line);
void header();

int main()
{
    char *line;
    char **args;
    int status;

    header();

    do
    {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line); 

        if (args[0] == NULL)
        {
            free(line);
            free(args);
            continue;
        }

        if (strcmp(args[0], "exit") == 0)
        {
            free(line);
            free(args);
            break;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            if (execvp(args[0], args) == -1)
            {
                perror("lsh");
            }
            exit(EXIT_FAILURE);
        }
        else if (pid < 0)
        {
            perror("lsh");
        }
        else
        {
            waitpid(pid, &status, 0);
            printf("\n");
        }

        free(line);
        free(args);

    } while (1);

    return 0;
}

void header() {
    printf(" ██████╗██████╗  █████╗ ███████╗██╗  ██╗\n");
    printf("██╔════╝██╔══██╗██╔══██╗██╔════╝██║  ██║\n");
    printf("██║     ██████╔╝███████║███████╗███████║\n");
    printf("██║     ██╔══██╗██╔══██║╚════██║██╔══██║\n");
    printf("╚██████╗██║  ██║██║  ██║███████║██║  ██║\n");
    printf(" ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝\n");
}

char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

char *lsh_read_line(void)
{
    char *line = NULL;
    ssize_t bufsize = 0;

    if (getline(&line, &bufsize, stdin) == -1)
    {
        if (feof(stdin))
        {
            exit(EXIT_SUCCESS);
        }
        else
        {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}