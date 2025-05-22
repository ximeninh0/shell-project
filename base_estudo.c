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

#define MAX_LINE 1024
#define MAX_ARGS 128
#define MAX_PATHS 64

char *paths[MAX_PATHS];
int path_count = 0;

void init_paths() {
    // default path /bin
    paths[0] = strdup("/bin");
    path_count = 1;
}

void print_error() {
    const char *msg = "An error has occurred\n";
    write(STDERR_FILENO, msg, strlen(msg));
}

// tokenize a line into args, return argc
int parse_args(char *line, char **args) {
    int argc = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && argc < MAX_ARGS-1) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;
    return argc;
}

// resolve external command via paths
char *resolve_cmd(char *cmd) {
    for (int i = 0; i < path_count; i++) {
        char buf[MAX_LINE];
        snprintf(buf, sizeof(buf), "%s/%s", paths[i], cmd);
        if (access(buf, X_OK) == 0) {
            return strdup(buf);
        }
    }
    return NULL;
}

// builtins
int builtin_exit(char **args) {
    if (args[1] != NULL) print_error();
    else exit(0);
    return 1;
}

int builtin_cd(char **args) {
    if (!args[1] || args[2]) {
        print_error();
    } else {
        if (chdir(args[1]) != 0) print_error();
    }
    return 1;
}

int builtin_pwd(char **args) {
    if (args[1]) { print_error(); return 1; }
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
    else print_error();
    return 1;
}

int builtin_path(char **args) {
    // clear old
    for (int i = 0; i < path_count; i++) free(paths[i]);
    path_count = 0;
    // set new
    for (int i = 1; args[i] && path_count < MAX_PATHS; i++) {
        paths[path_count++] = strdup(args[i]);
    }
    return 1;
}

int builtin_cat(char **args) {
    if (!args[1] || args[2]) { print_error(); return 1; }
    FILE *f = fopen(args[1], "r");
    if (!f) { print_error(); return 1; }
    char ch;
    while ((ch = fgetc(f)) != EOF) putchar(ch);
    fclose(f);
    return 1;
}

int builtin_ls(char **args) {
    int show_all = 0, long_fmt = 0;
    // parse flags
    for (int i = 1; args[i]; i++) {
        if (strcmp(args[i], "-a") == 0) show_all = 1;
        else if (strcmp(args[i], "-l") == 0) long_fmt = 1;
        else { print_error(); return 1; }
    }
    DIR *d = opendir(".");
    if (!d) { print_error(); return 1; }
    struct dirent *entry;
    while ((entry = readdir(d))) {
        if (!show_all && entry->d_name[0] == '.') continue;
        if (!long_fmt) {
            printf("%s  ", entry->d_name);
        } else {
            struct stat st;
            if (stat(entry->d_name, &st) == 0) {
                printf((S_ISDIR(st.st_mode)) ? "d" : "-");
                printf((st.st_mode & S_IRUSR) ? "r" : "-");
                printf((st.st_mode & S_IWUSR) ? "w" : "-");
                printf((st.st_mode & S_IXUSR) ? "x" : "-");
                printf(" %ld %s\n", st.st_size, entry->d_name);
            }
        }
    }
    if (!long_fmt) printf("\n");
    closedir(d);
    return 1;
}

int is_builtin(char *cmd) {
    return (!strcmp(cmd, "exit") || !strcmp(cmd, "cd") || !strcmp(cmd, "pwd")
        || !strcmp(cmd, "path") || !strcmp(cmd, "cat") || !strcmp(cmd, "ls"));
}

int run_builtin(char **args) {
    if (strcmp(args[0], "exit") == 0) return builtin_exit(args);
    if (strcmp(args[0], "cd") == 0) return builtin_cd(args);
    if (strcmp(args[0], "pwd") == 0) return builtin_pwd(args);
    if (strcmp(args[0], "path") == 0) return builtin_path(args);
    if (strcmp(args[0], "cat") == 0) return builtin_cat(args);
    if (strcmp(args[0], "ls") == 0) return builtin_ls(args);
    return 0;
}

// execute a simple command with possible redirection
void exec_simple(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        // child
        int fd;
        // handle output redirection
        for (int i = 0; args[i]; i++) {
            if (strcmp(args[i], ">") == 0) {
                if (!args[i+1] || args[i+2]) { print_error(); exit(1); }
                fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { print_error(); exit(1); }
                dup2(fd, STDOUT_FILENO);
                args[i] = NULL;
            }
        }
        // external?
        char *cmd_path = resolve_cmd(args[0]);
        if (!cmd_path) { print_error(); exit(1); }
        execv(cmd_path, args);
        print_error();
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        print_error();
    }
}

// parse and handle pipes and parallel
void eval_line(char *line) {
    // split parallel by '&'
    char *saveptr1;
    char *cmd = strtok_r(line, "&", &saveptr1);
    while (cmd) {
        // TODO: handle pipes: left as exercise
        // for now only simple commands
        char *parts = strdup(cmd);
        char *args[MAX_ARGS];
        int argc = parse_args(parts, args);
        if (argc == 0) { free(parts); cmd = strtok_r(NULL, "&", &saveptr1); continue; }
        if (is_builtin(args[0])) run_builtin(args);
        else exec_simple(args);
        free(parts);
        cmd = strtok_r(NULL, "&", &saveptr1);
    }
}

int main(int argc, char *argv[]) {
    init_paths();
    FILE *input = stdin;
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) { print_error(); exit(1); }
    } else if (argc > 2) {
        print_error(); exit(1);
    }
    char line[MAX_LINE];
    while (1) {
        char cwd[PATH_MAX];

        if (getcwd(cwd, sizeof(cwd)) != NULL){
            if (input == stdin) printf("%s sh> ", cwd);
            if (!fgets(line, sizeof(line), input)) break;
            eval_line(line);
        }
    }
    if (input != stdin) fclose(input);
    return 0;
}