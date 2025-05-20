#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

typedef struct
{
    const char *key;
    const char *value;
} Option;
// Serve como um hash simples

Option ls_options[] = {
    {"-l", "Listar"},
    {"-a", "Mostrar todos"},
    {NULL, NULL}};

const char *commands_with_args[] = {
    "cd",
    "echo",
    "cat",
    NULL};
const char *commands_without_args[] = {
    "dir",
    "exit",
    "pwd",
    "help",
    NULL};

const char *commands_with_flags[] = {
    "ls",
    NULL};

bool is_in_array(char *string, const char **vector);
const char *find_option(const char *key, Option *options);

char *lsh_read_line(void);
char **lsh_split_line(char *line);
void header();
void help();

void execute(char **args, int *status);

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

        bool is_command_without_args = is_in_array(args[0], commands_without_args);
        if (is_command_without_args)
        {
            if (strcmp(args[0], "exit") == 0)
            {
                free(line);
                free(args);
                break;
            }

            if (strcmp(args[0], "help") == 0)
            {
                help();
            } else {
                execute(args, &status);
            }
        }
        else
        {
            bool is_command_with_args = is_in_array(args[0], commands_with_args);
            if (is_command_with_args)
            {
                if (args[1] == NULL)
                {
                    printf("shell: Invalid arguments\n");
                    printf("usage: %s <args>\n", args[0]);
                } else {
                    // Verifica os argumentos ou só executa
                    execute(args, &status);
                }
            }
            else
            {
                bool is_command_with_flags = is_in_array(args[0], commands_with_flags);
                if(is_command_with_flags) {
                    // Verifica os argumentos
                    execute(args, &status);
                } else {
                    // Executa um arquivo ou da erro
                }
            }
        }

        free(line);
        free(args);

    } while (1);

    return 0;
}

void header()
{
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

bool is_in_array(char *string, const char **vector)
{
    bool is_in = false;
    for (int i = 0; vector[i] != NULL; i++)
    {
        if (strcmp(vector[i], string) == 0)
        {
            is_in = true;
        }
    }
    return is_in;
}

const char *find_option(const char *key, Option *options)
{
    for (int i = 0; options[i].key != NULL; i++)
    {
        if (strcmp(options[i].key, key) == 0)
            return options[i].value;
    }
    return NULL;
}

void execute(char **args, int *status)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
        {
            perror("shell");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("shell");
    }
    else
    {
        waitpid(pid, status, 0);
        printf("\n");
    }
}
void help()
{
    printf("Help Command");
}