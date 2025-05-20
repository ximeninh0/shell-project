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
// Serve como um dicionário simples

typedef struct
{
    const char *command;
    const Option *flags;
} CommandFlags;

const Option ls_options[] = {
    {"-l", "Listar"},
    {"-a", "Mostrar todos"},
    {NULL, NULL}};

const CommandFlags commands_with_flags[] = {
    {"ls", ls_options},
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

bool is_in_array(char *string, const char **vector);
bool is_in_command_flag(char *string, const CommandFlags *commands);
const char *find_option(const char *key, const Option *options);
const CommandFlags *find_command(const char *command, const CommandFlags *commands);

char *lsh_read_line(void);
char **lsh_split_line(char *line);
void header();
void small_header();
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
                continue;
            }

            execute(args, &status);
        }
        else
        {
            bool is_command_with_args = is_in_array(args[0], commands_with_args);
            if (is_command_with_args)
            {
                if (args[1] == NULL)
                {
                    printf("crash: Invalid arguments\n");
                    printf("usage: %s <args>\n", args[0]);
                    continue;
                }
                // Verifica os argumentos ou só executa
                execute(args, &status);
            }
            else
            {
                const CommandFlags *command = find_command(args[0], commands_with_flags);
                if (command != NULL)
                {
                    const Option *flags = command->flags;
                    bool is_valid = true;
                    for (int i = 1; args[i] != NULL && is_valid == true; i++)
                    {
                        const char *opt = find_option(args[i], flags);
                        if (opt == NULL)
                            is_valid = false;
                    }
                    if (!is_valid)
                    {
                        printf("crash: invalid flags\n");
                        continue;
                    }
                    execute(args, &status);
                    continue;
                }

                // Executa um arquivo ou da erro
                printf("crash: invalid command\n");
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
        fprintf(stderr, "crash: allocation error\n");
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
                fprintf(stderr, "crash: allocation error\n");
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

const char *find_option(const char *key, const Option *options)
{
    for (int i = 0; options[i].key != NULL; i++)
    {
        if (strcmp(options[i].key, key) == 0)
        {
            return options[i].value;
        }
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
            perror("crash");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("crash");
    }
    else
    {
        waitpid(pid, status, 0);
        printf("\n");
    }
}
void help()
{
    small_header();

    printf("Comandos disponíveis:\n\n");
    printf("Comandos sem argumentos:\n");
    for (int i = 0; commands_without_args[i] != NULL; i++)
    {
        printf("- %s\n", commands_without_args[i]);
    }
    printf("Comandos com argumentos:\n");
    for (int i = 0; commands_with_args[i] != NULL; i++)
    {
        printf("- %s\n", commands_with_args[i]);
    }
    printf("Comandos com flags:\n");
    for (int i = 0; commands_with_flags[i].command != NULL; i++)
    {
        printf("- %s\n", commands_with_flags[i].command);
        const Option *flags = commands_with_flags[i].flags;
        for (int j = 0; flags[j].key != NULL; j++)
        {
            printf("\t%s -> %s\n", flags[j].key, flags[j].value);
        }
    }
}
void small_header()
{
    printf("CRASH\n");
}
bool is_in_command_flag(char *string, const CommandFlags *commands)
{
    const CommandFlags *command = find_command(string, commands);
    return (command != NULL);
}

const CommandFlags *find_command(const char *command, const CommandFlags *commands)
{
    for (int i = 0; commands[i].command != NULL; i++)
    {
        if (strcmp(commands[i].command, command) == 0)
        {
            return &commands[i];
        }
    }
    return NULL;
}

/**
 * To Do
 *
 * - Verificar os argumentos
 * - Verificar os flags
 * - Executar um arquivo
 * - Lançar um erro se o comando ou arquivo não existir
 */
