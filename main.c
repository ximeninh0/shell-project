#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

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

void pwd(){
    char cwd[PATH_MAX];  // como se fosse um buffer para armazenar os caminhos | PATH_MAX eh o tamanho maximo de um caminho geralmente 4096, pelo menis em linux
    if (getcwd(cwd, sizeof(cwd)) != NULL) { // getcwd eh uma funcao de unistd que chama o diretorio como 
        printf("diretorio atual: %s\n", cwd);
    } else {
        perror("pwd");
    }
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

int main()
{
    char *input;

    char *line;
    char **args;
    int status;

    while (strcmp(line, "exit") != 0)
    {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        printf("Comando executado: %s\n", line);

        if (strcmp(line, "pwd") == 0){
            pwd();
            continue;
        }
    }
    printf("\n\n%s\n", line);

    printf("Hello World!");
    return 0;
}