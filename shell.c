#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdbool.h>

// ! Definições de constantes
#define MAX_LINE 1024   // tamanho máximo da linha de entrada
#define MAX_PROCS 10    // número máximo de processos (separados por &)
#define MAX_ARGS 20     // número máximo de argumentos por processo
#define BUFFER_SIZE 256 // tamanho máximo da linha de entrada
#define MAX_STAGES 10   // número máximo de estágios em um pipeline

// ! Estrutura para lista encadeada de caminhos
typedef struct element
{
    char *valor;
    struct element *prox;
} Lista;

// ! Prototipos de funcoes principais do shell
int simultaneos_proc(char *input, char *out_args[MAX_PROCS][MAX_ARGS + 1]);
int split_pipeline_args(char *in_args[], char *out_args[MAX_STAGES][MAX_ARGS + 1]);
void print_args(char *row[]);
void execute(char **args, Lista *paths_list);
void execute_pipeline(char *stages[MAX_STAGES][MAX_ARGS + 1], int stage_count, Lista *paths_list);
pid_t launch_process(int in_fd, int out_fd, char **args, Lista *paths_list);
int count_args(char **args);
bool validate_command(char **args);
int handle_output_file(char **args, char **output_file);
void remove_output_file(char **args);
int find_and_exec_command(char **args, Lista *paths_list);
void help();
void process_command_line(char **command_group, Lista **paths);
bool handle_builtin_command(char **args, Lista **paths);

// ! Funções auxiliares para manipulação da lista de caminhos
Lista *init();
Lista *insert(Lista *receba, char valor[]);
void handle_path_command(char **args, Lista **paths_list);
Lista *remove_by_value(Lista *head, const char *value_to_remove);
Lista *liberaListaAndReset(Lista *head);
int isEmpty(Lista *list);
void printAll(Lista *p);

int main(int argc, char *argv[])
{
    Lista *paths = init(); // Inicializa a lista de caminhos

    char line[MAX_LINE]; // Buffer para armazenar a linha de entrada
    char cwd[PATH_MAX]; // Buffer para armazenar o diretório atual
    char *args[MAX_PROCS][MAX_ARGS + 1]; // Array para armazenar os argumentos dos processos
    int procs = 0; // Número de processos simultâneos

    FILE *input = stdin; // Entrada padrão

    while (1)
    {
        if (getcwd(cwd, sizeof(cwd)) == NULL) // Obtém o diretório atual
            break;

        if (input == stdin) // Se a entrada for padrão, imprime o prompt
            printf("%s $: ", cwd);

        if (fgets(line, sizeof(line), input) == NULL) // Lê uma linha da entrada
            break;

        if (line[0] == '\n' || line[0] == '\0') // Linha vazia
            continue;

        procs = simultaneos_proc(line, args); // Separa os processos simultâneos

        for (int p = 0; p < procs; p++) // Processa cada processo e pipeline com argumentos
            process_command_line(args[p], &paths);
    }

    printf("\nSaindo do shell...\n");
    return 0;
}

// ! Função que separa e retorna o numero de processos simultaneos separados por &
int simultaneos_proc(char *input, char *out_args[MAX_PROCS][MAX_ARGS + 1])
{
    char *procs[MAX_PROCS];
    int proc_count = 0;

    // Remove o \n final
    input[strcspn(input, "\n")] = '\0';

    // Separar por '&'
    char *saveptr_proc;
    char *tok_proc = strtok_r(input, "&", &saveptr_proc);
    while (tok_proc && proc_count < MAX_PROCS)
    {
        // Trim espaços iniciais
        while (*tok_proc == ' ')
            tok_proc++;
        // Trim espaços finais
        char *end = tok_proc + strlen(tok_proc) - 1;
        while (end > tok_proc && *end == ' ')
        {
            *end = '\0';
            end--;
        }
        procs[proc_count++] = tok_proc;
        tok_proc = strtok_r(NULL, "&", &saveptr_proc);
    }

    // Para cada processo, separar em argumentos
    for (int p = 0; p < proc_count; p++)
    {
        int argc = 0;
        char *saveptr_arg;
        char *tok_arg = strtok_r(procs[p], " \t", &saveptr_arg);
        while (tok_arg && argc < MAX_ARGS)
        {
            out_args[p][argc++] = tok_arg;
            tok_arg = strtok_r(NULL, " \t", &saveptr_arg);
        }
        out_args[p][argc] = NULL;
    }

    return proc_count; // Retorna o número de processos encontrados
}

// ! funcao para debugar os args
void print_args(char *row[])
{
    if (row == NULL)
    {
        printf("Erro: row é NULL\n");
        return;
    }

    for (int i = 0; row[i] != NULL; i++)
    {
        printf("%s", row[i]);
        if (row[i + 1] != NULL)
            printf(" "); // espaço entre argumentos
    }
    printf("\n");
}

// ! Função que separa e retorna o num de processos "dependentes"/pipeline separados por |
int split_pipeline_args(char *in_args[], char *out_args[MAX_STAGES][MAX_ARGS + 1])
{
    int stage = 0, argc = 0;

    for (int i = 0; in_args[i] != NULL && stage < MAX_STAGES; i++)
    {
        if (strcmp(in_args[i], "|") == 0)
        {
            // fecha o estagio atual
            out_args[stage][argc] = NULL;
            stage++;
            argc = 0;
        }
        else
        {
            // adiciona token ao estagio atual
            if (argc < MAX_ARGS)
            {
                out_args[stage][argc++] = in_args[i];
            }
        }
    }
    // termina o ultimo estagio
    out_args[stage][argc] = NULL;
    return stage + 1;
}

// ! Função que executa comando simples
void execute(char **args, Lista *paths_list)
{
    char *output_file = NULL; // Variável para armazenar o nome do arquivo de saída, se houver
    int out_fd = STDOUT_FILENO; // Descritor de arquivo para saída padrão
    int status;

    // Trata o comando de redirecionamento para arquivo, se houver
    status = handle_output_file(args, &output_file);

    if (status == -1)
        return; // erro de sintaxe

    // Se houver um arquivo de saída, abre-o para escrita, se não, usa a saída padrão
    if (output_file != NULL)
    {
        out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0)
        {
            perror("erro ao abrir o aquivo\n");
            return;
        }
    }

    // Lança o processo com os argumentos fornecidos
    pid_t pid = launch_process(STDIN_FILENO, out_fd, args, paths_list);

    if (out_fd != STDOUT_FILENO)
    {
        close(out_fd);
    }

    if (pid > 0)
    {
        waitpid(pid, NULL, 0);
    }
}

// ! cria e lanca o processo com pipes para comunicacao com outro processo
pid_t launch_process(int in_fd, int out_fd, char **args, Lista *paths_list)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork error");
        return -1; // Retorna -1 para indicar erro no fork
    }

    if (pid == 0)
    {                              // Processo Filho
        if (in_fd != STDIN_FILENO) // redireciona para entrada padrao
        {
            if (dup2(in_fd, STDIN_FILENO) < 0)
            {
                perror("dup2 input error");
                exit(EXIT_FAILURE);
            }
            close(in_fd);
        }

        // Redireciona a saida padrão se nao for STDOUT_FILENO, so sai para padrao se for o ultimo comando
        if (out_fd != STDOUT_FILENO)
        {
            if (dup2(out_fd, STDOUT_FILENO) < 0)
            {
                perror("dup2 output error");
                exit(EXIT_FAILURE);
            }
            close(out_fd); // Fecha o descritor original
        }

        // Verifica se o comando está na lista de paths
        if (find_and_exec_command(args, paths_list) == -1)
        {
            perror("Erro ao executar o comando");
            exit(EXIT_FAILURE);
        }
    }

    return pid; // Processo Pai retorna o PID do filho
}

// ! executa os comandos juntos chamando launch_process juntamente com pipes
void execute_pipeline(char *stages[MAX_STAGES][MAX_ARGS + 1], int stage_count, Lista *paths_list)
{
    int in_fd = STDIN_FILENO; // Descritor de arquivo para entrada padrão
    int fd[2]; // Descritores para o pipe
    pid_t pids[MAX_STAGES]; // Array para armazenar os PIDs dos processos filhos
    char *output_file = NULL; // Variável para armazenar o nome do arquivo de saída, se houver
    int status;
    int last_out_fd = STDOUT_FILENO; // Descritor de arquivo para a saída do último comando

    for (int i = 0; i < stage_count; i++)
    {
        int out_fd;

        // Se nao for o ultimo comando, cria um pipe para a saida
        if (i < stage_count - 1)
        {
            remove_output_file(stages[i]); // remove o comando de redirecionamento para arquivo, pois nao eh suportado em pipeline complexa

            if (pipe(fd) == -1)
            {
                perror("pipe error");
                exit(EXIT_FAILURE);
            }
            out_fd = fd[1]; // A saida sera a escrita do pipe
        }
        else // ultimo caso, escreve na saida padrao ou em um arquivo
        {
            // Trata o comando de redirecionamento para arquivo, se houver
            status = handle_output_file(stages[i], &output_file);
            if (status == -1)
            {
                fprintf(stderr, "erro de sintaxa abortando\n");
                return;
            }

            // Se houver um arquivo de saída, abre-o para escrita, se não, usa a saída padrão
            if (output_file != NULL)
            {
                out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if (out_fd < 0)
                {
                    perror("error ao abrir pipe de saida");
                    close(out_fd);
                    return;
                }
            }
            else // se nao houver arquivo de saida, usa a saida padrao
            {
                out_fd = STDOUT_FILENO;
            }
        }

        // Lança o processo para o "comando atual"
        pids[i] = launch_process(in_fd, out_fd, stages[i], paths_list);

        // Fecha os "pipes de escrita" no processo pai
        if (in_fd != STDIN_FILENO)
            close(in_fd);

        if (out_fd != STDOUT_FILENO)
            close(out_fd);

        // A entrada para o proximo comando sera a leitura do pipe atual
        if (i < stage_count - 1)
            in_fd = fd[0];
    }

    for (int i = 0; i < stage_count; i++)
        if (pids[i] > 0)
            waitpid(pids[i], NULL, 0);
}

// ! conta a quantidade de argumentos em cada comando
int count_args(char **args)
{
    int count = 0;
    if (args == NULL)
        return 0;

    while (args[count] != NULL)
    {
        count++;
    }
    return count;
}

// ! verifica se o comando e valido e se seus argumentos sao validos
bool validate_command(char **args)
{

    char *command = args[0];
    int num_args = count_args(args) - 1;

    if (strcmp(command, "cd") == 0)
    {
        if (num_args != 1)
        {
            fprintf(stderr, "uso: cd <dretorio>\n");
            return false;
        }
        return 1;
    }
    else if (strcmp(command, "ls") == 0)
    {
        return true;
    }
    else if (strcmp(command, "pwd") == 0)
    {
        if (num_args != 0)
        {
            fprintf(stderr, "uso: pwd\n");
            return false;
        }
        return true;
    }
    else if (strcmp(command, "cat") == 0)
    {
        if (num_args < 1)
        {
            fprintf(stderr, "uso: cat <arquivo> [arquivo2 ...]\n");
            return false;
        }
        return true;
    }
    else
    {
        return true;
    }
}

// ! aponta output_file para o arquivo apos > colocando em pipes
int handle_output_file(char **args, char **output_file)
{
    *output_file = NULL;

    // Verifica se há um comando de redirecionamento para arquivo
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], ">") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "erro: falta nome do arquivo apos > \n");
                return -1; // Indica a falha
            }

            *output_file = args[i + 1];

            args[i] = NULL;

            return 1;
        }
    }

    return 0; // Nao ha > nos argumentos
}

// ! remove o comando de redirecionamento para arquivo, pois nao eh suportado em pipeline complexa
void remove_output_file(char **args)
{
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], ">") == 0)
        {
            fprintf(stderr, "avido: redirecionamento para arquivo nao eh suportado em pipeline complexa\n");
            args[i] = NULL;     // Remove o comando de redirecionamento
            args[i + 1] = NULL; // Remove o arquivo de saida
            break;
        }
    }
}

// ! Função que encontra e executa o comando, seja por caminho absoluto/relativo ou pelo PATH
int find_and_exec_command(char **args, Lista *paths_list)
{
    // Caso 1: Comando já é um caminho absoluto ou relativo (e.g., "/bin/ls" ou "./my_script")
    // Se o primeiro argumento contém '/', tenta executá-lo diretamente.
    if (strchr(args[0], '/') != NULL)
    {
        if (execv(args[0], args) == -1)
        {
            // Se falhar, imprime o erro específico (ex: "No such file or directory")
            perror(args[0]);
            return -1; // Indica que não foi possível executar
        }
        // execv só retorna se houver erro, caso contrário, o processo é substituído.
    }

    // Caso 2: Comando precisa ser buscado nos diretórios do PATH
    // (O comando não contém '/', e.g., "ls", "cat", "sum")
    char full_path[PATH_MAX]; // Buffer para construir o caminho completo

    Lista *current_path_node = paths_list; // Ponteiro para percorrer sua lista de PATHs
    while (current_path_node != NULL)
    {
        // Constrói o caminho completo: "diretorio_do_path" + "/" + "comando"
        // snprintf é mais seguro que sprintf pois evita buffer overflow
        snprintf(full_path, PATH_MAX, "%s/%s", current_path_node->valor, args[0]);

        // Verifica se o arquivo existe e tem permissão de execução
        if (access(full_path, X_OK) == 0)
        {
            // Encontrou o executável! Tenta executá-lo.
            if (execv(full_path, args) == -1)
            {
                // Se execv falhar (por exemplo, arquivo corrompido, permissão negada APÓS access)
                perror(full_path); // Imprime o erro específico
                return -1;         // Indica falha na execução
            }
            // execv só retorna em caso de erro. Se chegou aqui, o processo foi substituído.
        }
        current_path_node = current_path_node->prox; // Vai para o próximo diretório na lista
    }

    // Se o loop terminou e o comando não foi encontrado em nenhum PATH
    fprintf(stderr, "%s: comando não encontrado\n", args[0]);
    return -1; // Indica que o comando não foi encontrado/executado
}

// iniciar lista
Lista *init()
{
    return NULL;
}

// inserir novo elemento na lista
Lista *insert(Lista *receba, char valor[])
{
    Lista *novo = (Lista *)malloc(sizeof(Lista));
    if (novo == NULL)
    {
        perror("Erro");
        return receba;
    }

    novo->valor = strdup(valor);
    novo->prox = NULL; // O novo nó será o último

    if (receba == NULL)
    {
        return novo;
    }
    else
    {
        // Percorre até o último nó
        Lista *atual = receba;
        while (atual->prox != NULL)
        {
            atual = atual->prox;
        }
        atual->prox = novo; // Anexa o novo nó ao final
        return receba;      // Retorna a cabeça original
    }
}

// liberar lista toda
Lista *liberaListaAndReset(Lista *head)
{
    Lista *current = head;
    Lista *next_node;
    while (current != NULL)
    {
        next_node = current->prox;
        free(current->valor);
        free(current);
        current = next_node;
    }
    return NULL; // Retorna NULL para a nova cabeça da lista (vazia)
}

// ! Função que trata o comando "path" para manipular a lista de caminhos
void handle_path_command(char **args, Lista **paths_list)
{
    int argc = count_args(args);

    if (argc == 1)
    {
        printf("PATH atual do shell:\n");
        printAll(*paths_list);
        return;
    }

    if (argc >= 3 && strcmp(args[1], "+") == 0)
    {
        for (int i = 2; i < argc; i++)
        {
            *paths_list = insert(*paths_list, args[i]);
            printf("Adicionado: %s\n", args[i]);
        }
        return;
    }

    if (argc >= 3 && strcmp(args[1], "-") == 0)
    {
        for (int i = 2; i < argc; i++)
        {
            *paths_list = remove_by_value(*paths_list, args[i]);
            // A função remove_by_value pode ser silenciosa ou modificada para retornar status
            printf("Tentativa de remover: %s\n", args[i]);
        }
        return;
    }

    // Se nenhuma das opções acima, mostra o uso correto
    fprintf(stderr, "Uso: path [+ /novo/caminho] [- /caminho/a/remover]\n");
}

// ! remove um elemento da lista pelo valor
Lista *remove_by_value(Lista *head, const char *value_to_remove)
{
    if (head == NULL)
        return NULL;

    while (head != NULL && strcmp(head->valor, value_to_remove) == 0)
    {
        Lista *temp = head;
        head = head->prox;
        free(temp->valor);
        free(temp);
    }

    // Se a lista ficou vazia
    if (head == NULL)
        return NULL;

    Lista *current = head;
    while (current->prox != NULL)
    {
        if (strcmp(current->prox->valor, value_to_remove) == 0)
        {
            Lista *temp = current->prox;
            current->prox = temp->prox; // "Pula" o nó a ser removido
            free(temp->valor);
            free(temp);
        }
        else
        {
            current = current->prox; // Avança apenas se não houve remoção
        }
    }

    return head;
}

// verifica se a lista esta vazia
int isEmpty(Lista *list)
{
    if (list == NULL)
        return 1;
    return 0;
}

// debugar lista de paths
void printAll(Lista *p)
{
    int cont = 0;
    if (isEmpty(p))
    {
        printf("\nA lista esta vazia!\n");
        return;
    }

    while (p != NULL)
    {
        printf("\nElemento%d: %s ", cont + 1, p->valor);
        p = p->prox;
        cont++;
    }
}

// ! Função que exibe a ajuda do shell
void help()
{
    printf("Comandos suportados:\n");
    printf(" - Redirecionamento de saída com > é suportado.\n");
    printf(" - Comandos podem ser encadeados com & e | para execução simultânea ou em pipeline.\n");
    printf(" - exit: Sai do shell.\n");
    printf(" - cd <diretorio>: Muda o diretório atual.\n");
    printf(" - pwd: Mostra o diretório atual.\n");
    printf(" - path <diretorios>: Define os diretórios onde procurar comandos.\n");
    printf(" - ls: Lista arquivos no diretório atual.\n");
    printf(" - cat <arquivo>: Exibe o conteúdo de um arquivo.\n");
    printf(" - Comandos podem ser executados com caminhos absolutos ou relativos.\n");
    printf(" - Argumentos devem ser separados por espaços, ex: ls -a -l.\n");
    printf(" - Para executar outros programas, use o caminho completo ou defina o PATH com o comando path.\n");
}

// ! Função que trata comandos internos do shell (built-in commands)
bool handle_builtin_command(char **args, Lista **paths)
{
    char *command = args[0];

    if (strcmp(command, "exit") == 0)
    {
        printf("Saindo do shell...\n");
        liberaListaAndReset(*paths);
        exit(0);
    }
    else if (strcmp(command, "cd") == 0)
    {
        if (chdir(args[1]) != 0)
        {
            perror("Erro ao mudar de diretório");
        }

        return true; // cd foi tratado
    }
    else if (strcmp(command, "path") == 0)
    {
        handle_path_command(args, paths);
        printf("\nCaminhos (paths) atualizados. Use 'path' para ver a lista.\n"); 
        return true;
    }
    else if (strcmp(command, "help") == 0)
    {
        help();
        return true; // help foi tratado
    }

    return false; // Não era um built-in conhecido
}

void process_command_line(char **command_group, Lista **paths)
{
    char *pipeline_args[MAX_STAGES][MAX_ARGS + 1];
    int stage_count = split_pipeline_args(command_group, pipeline_args);

    if (stage_count <= 0 || pipeline_args[0][0] == NULL)
    {
        // Pipeline inválido ou vazio
        return;
    }

    if (stage_count > 2)
    {
        fprintf(stderr, "Erro: pipeline com mais de 2 comandos nao suportada nesta versão.\n");
        return;
    }

    for (int s = 0; s < stage_count; s++)
    {
        if (!validate_command(pipeline_args[s]))
        {
            return; // Interrompe se a sintaxe de um comando for inválida
        }

        char *cmd = pipeline_args[s][0];
        if (stage_count > 1 && (strcmp(cmd, "exit") == 0 || strcmp(cmd, "cd") == 0 || strcmp(cmd, "path") == 0 || strcmp(cmd, "help") == 0))
        {
            fprintf(stderr, "Erro: O comando '%s' não pode ser executado em um pipeline.\n", cmd);
            return; // Interrompe se um built-in inválido for encontrado no pipeline
        }
    }

    if (stage_count == 1)
    {
        if (!handle_builtin_command(pipeline_args[0], paths))
        {
            execute(pipeline_args[0], *paths);
        }
    }
    else
    {
        execute_pipeline(pipeline_args, stage_count, *paths);
    }
}
