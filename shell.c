#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdbool.h>

#define MAX_LINE 1024
#define MAX_PROCS 10    // número máximo de processos (separados por &)
#define MAX_ARGS 20     // número máximo de argumentos por processo
#define BUFFER_SIZE 256 // tamanho máximo da linha de entrada
#define MAX_STAGES 10

int simultaneos_proc(char *input, char *out_args[MAX_PROCS][MAX_ARGS + 1]);
int split_pipeline_args(char *in_args[], char *out_args[MAX_STAGES][MAX_ARGS + 1]);
void print_args(char *row[]);
int is_builtin(char *comand);
void execute(char **args);
void execute_pipeline(char *stages[MAX_STAGES][MAX_ARGS + 1], int stage_count);
pid_t launch_process(int in_fd, int out_fd, char **args);
int count_args(char **args);
bool validate_command(char **args);
int handle_output_file(char ** args, char **output_file);
void fillPathsList(char **args,Lista*paths);

typedef struct element{
    char valor[MAX_STAGES];
    struct element *prox;
}Lista;

Lista* init();
Lista* insert(Lista* receba,char valor[MAX_ARGS]);
Lista* removeFrom(Lista* deleted);
void printAll(Lista *p);

// TODO validacao de erros, help, comandos exigidos pelo denis como cd, ls, ...
// TODO comando cd atualmente nao funcion, utilizar a fun is_builtin para tratar e executa-lo
// TODO verificar tbm se os comandos chamados podem ser executados juntos e se precisam de argumentos

int main(int argc, char *argv[])
{
    Lista* paths;
    paths = init();
    char line[MAX_LINE];
    char cwd[PATH_MAX]; // salva o current working dir
    char *args[MAX_PROCS][MAX_ARGS + 1]; // slip inicial da linha separando em comandos "simultaneos"
    char *pipe_args[MAX_STAGES][MAX_ARGS + 1]; // contem o que deve ser executado
    int stage_count;
    int procs = 0;

    FILE *input = stdin; // para leitura constante do stdin

    while (1)
    {
        printAll(paths);
        fflush(stdin);
        stage_count = 0; // # stage_count contem o numero de proc de devem ser executados via pipe onde: proc_1 > stout >> proc_2 >stdin

        if (getcwd(cwd, sizeof(cwd)) == NULL) // serve apenas para pegar o diretorio atual 
            break;

        if (input == stdin) // printa o dir atual antes de pedir entrada
            printf("%s $: ", cwd);

        if (fgets(line, sizeof(line), input) == NULL)
            break; // ! sai em caso de erro na leitura

        if (line[0] == '\n' || line[0] == '\0')
            continue;

        procs = simultaneos_proc(line, args);

        for (int p = 0; p < procs; p++)
        {
            stage_count = split_pipeline_args(args[p], pipe_args);

            bool pipe_is_valid = true;

            if(strcmp(pipe_args[0][0], "path") == 0){
                fillPathsList(args,paths);
                continue;
            }
            for (int s = 0; s < stage_count; s++)
            {
                if (pipe_args[s][0] == NULL)
                {
                    fprintf(stderr, "Erro: pipeline vazia\n");
                    continue;;
                }
                
                if (!validate_command(pipe_args[s]))
                {
                    pipe_is_valid = false;
                    continue;
                }
            }

            if (pipe_is_valid)
            {    
                if (stage_count > 1)
                {
                    execute_pipeline(pipe_args, stage_count);
                }
                else
                {
                    execute(pipe_args[0]);
                    continue;
                }
            }
        }
    }
}

// separa e retorna o numero de processos simultaneos separados por &
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
        out_args[p][argc] = NULL; // argv-style
    }

    return proc_count;
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

// separa e retorna o num de processos "dependentes" separados por |
int split_pipeline_args(char *in_args[], char *out_args[MAX_STAGES][MAX_ARGS + 1])
{
    int stage = 0, argc = 0;

    for (int i = 0; in_args[i] != NULL && stage < MAX_STAGES; i++)
    {
        if (strcmp(in_args[i], "|") == 0)
        {
            // fecha o estágio atual
            out_args[stage][argc] = NULL;
            stage++;
            argc = 0;
        }
        else
        {
            // adiciona token ao estágio atual
            if (argc < MAX_ARGS)
            {
                out_args[stage][argc++] = in_args[i];
            }
        }
    }
    // termina o último estágio
    out_args[stage][argc] = NULL;
    return stage + 1;
}

// ! func que executa comando simples, no caso comandos seperados por &
void execute(char **args)
{
    char *output_file = NULL;
    int out_fd = STDOUT_FILENO;
    int status;

    status = handle_output_file(args, &output_file);

    if (status == -1) return; // erro de sintaxe

    if (output_file != NULL)
    {
        out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0)
        {
            perror("erro ao abrir o aquivo\n");
            return;
        }
    }

    pid_t pid = launch_process(STDIN_FILENO, out_fd, args);

    if (out_fd != STDOUT_FILENO)
    {
        close(out_fd);
    }
    
    if (pid > 0 )
    {
        waitpid(pid, NULL, 0);
    }
}

// ! cria e lanca o processo com pipes para comunicacao com outro processo
pid_t launch_process(int in_fd, int out_fd, char **args)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork error");
        return -1; // Retorna -1 para indicar erro no fork
    }

    if (pid == 0)
    { // Processo Filho
        if (in_fd != STDIN_FILENO) // redireciona para entrada padrao
        {
            if (dup2(in_fd, STDIN_FILENO) < 0)
            {
                perror("dup2 input error");
                exit(EXIT_FAILURE);
            }
            close(in_fd); 
        }

        // Redireciona a saida padrão se não for STDOUT_FILENO, so sai para padrao se for o ultimo comando
        if (out_fd != STDOUT_FILENO)
        {
            if (dup2(out_fd, STDOUT_FILENO) < 0)
            {
                perror("dup2 output error");
                exit(EXIT_FAILURE);
            }
            close(out_fd); // Fecha o descritor original
        }

        // Executa o comando e sai caso tenha erro
        if (execvp(args[0], args) == -1)
        {
            perror("Erro ao executar o comando");
            exit(EXIT_FAILURE);
        }
    }

    return pid; // Processo Pai retorna o PID do filho
}

// ! executa os comandos juntos chamando launch_process juntamente com pipes
void execute_pipeline(char *stages[MAX_STAGES][MAX_ARGS + 1], int stage_count)
{
    int in_fd = STDIN_FILENO;
    int fd[2];
    pid_t pids[MAX_STAGES];
    char *output_file = NULL;
    int status;
    int last_out_fd = STDOUT_FILENO;

    for (int i = 0; i < stage_count; i++)
    {
        int out_fd;

        // Se nao for o último comando, cria um pipe para a saida
        if (i < stage_count - 1)
        {
            if (pipe(fd) == -1)
            {
                perror("pipe error");
                exit(EXIT_FAILURE);
            }
            out_fd = fd[1]; // A saida sera a escrita do pipe
        }
        else // ultimo caso, escreve na saida padrao
        {
            status = handle_output_file(stages[i], &output_file); 
            if (status == -1)
            {
                fprintf(stderr, "erro de sintaxa abortando\n");
                return;
            }
            
            if (output_file != NULL)
            {
                out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if (out_fd < 0)
                {
                    perror("error ao abrir pipe de saida");
                    close(out_fd);
                    return;
                }
            }else 
            {
                out_fd = STDOUT_FILENO;
            }
        }

        // Lança o processo para o "comando atual"
        pids[i] = launch_process(in_fd, out_fd, stages[i]);

        // Fecha os "pipes de escrita" no processo pai
        if (in_fd != STDIN_FILENO) close(in_fd);
        
        if (out_fd != STDOUT_FILENO) close(out_fd);

        // A entrada para o proximo comando sera a leitura do pipe atual
        if (i < stage_count - 1) in_fd = fd[0];
    }

    for (int i = 0; i < stage_count; i++)
        if (pids[i] > 0) waitpid(pids[i], NULL, 0);
}

// ! conta a quantidade de argumentos em cada comando
int count_args(char **args)
{
    int count = 0;
    if(args == NULL) return 0;

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
    }else if (strcmp(command, "ls") == 0)
    {
        return true;
    }else if (strcmp(command, "pwd") == 0)
    {
        if (num_args != 0)
        {
            fprintf(stderr, "uso: pwd\n");
            return false;
        }
        return true;
    }else if (strcmp(command, "cat") == 0)
    {
        if (num_args < 1)
        {
            fprintf(stderr, "uso: cat <arquivo> [arquivo2 ...]\n");
            return false;
        }
        return true;
    }else
    {
        return true;
    }
}

// ! aponta output_file para o arquivo apos > colocando em pipes
int handle_output_file(char ** args, char **output_file)
{
    *output_file = NULL;

    for (int i = 0; args[i] != NULL; i++)
    {
        if(strcmp(args[i], ">") == 0)
        {
            if (args[i+1] == NULL)
            {
                fprintf(stderr, "erro: falta nome do arquivo apos > \n");
                return -1; // Indica a falha
            }

            *output_file = args[i+1];

            args[i] = NULL;

            return 1;
        }
    }
    
    return 0; // Nao ha > nos argumentos
}

//Lida com o comando path
void fillPathsList(char* args[MAX_STAGES][MAX_ARGS],Lista*paths){
    liberaLista(paths);
    for(int i = 1; i < count_args(args);i++){
        printf("%s ",args[0][i]);
        insert(paths,args[0][i]);
    }
    printf("passou");
}

//iniciar lista
Lista* init(){
    return NULL;
}

//inserir novo elemento na lista
Lista* insert(Lista* receba, char valor[]){
    Lista* novo;
    novo = (Lista*)malloc(sizeof(Lista));
    strcpy(novo->valor,valor);
    novo->prox = receba;
    return novo;
}

//remover elemento da lista
Lista* removeFrom(Lista* init){
    Lista* novo;
    init = init->prox;
    return init;
}

//liberar lista toda
void liberaLista(Lista* list){
    Lista* aux = list;
    Lista* prox;
    while(aux != NULL){
        free(aux->prox);
        free(aux);
    }
}
//verifica se a lista esta vazia
int isEmpty(Lista* list){
    if(list == NULL)
        return 1;
    return 0;
}

//debugar lista de paths
void printAll(Lista *p){
    int cont=0;
    if(isEmpty(p)){
        printf("\nA lista esta vazia!\n");
        return;
    }
    
    while(p != NULL){
        printf("\nElemento%d: %s ",cont+1,p->valor);
        p = p->prox;
        cont++;
    }
}