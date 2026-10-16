#include "forge.h"
int builtin_index(const char *name) {
    static const char *names[] = {"cd", "pwd", "export", "exit", "history", "jobs", "fg", "bg", "help"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        if (strcmp(name, names[i]) == 0)
            return (int)i;
    return -1;
}
int run_builtin(Command *command, bool *should_exit) {
    const char *name = command->argv[0];
    char **args = command->argv;
    size_t argc = command->argc;
    if (strcmp(name, "cd") == 0) {
        const char *destination = argc > 1 ? args[1] : getenv("HOME");
        if (!destination) {
            fprintf(stderr, "forge: cd: HOME is not set\n");
            return 1;
        }
        if (strcmp(destination, "-") == 0) {
            if (!previous_directory) {
                fprintf(stderr, "forge: cd: previous directory is not set\n");
                return 1;
            }
            destination = previous_directory;
        }
        char *old = getcwd(NULL, 0);
        if (chdir(destination) < 0) {
            perror("forge: cd");
            free(old);
            return 1;
        }
        free(previous_directory);
        previous_directory = old;
        if (argc > 1 && strcmp(args[1], "-") == 0) {
            char *cwd = getcwd(NULL, 0);
            if (cwd) {
                puts(cwd);
                free(cwd);
            }
        }
    } else if (strcmp(name, "pwd") == 0) {
        char *cwd = getcwd(NULL, 0);
        if (!cwd) {
            perror("forge: pwd");
            return 1;
        }
        puts(cwd);
        free(cwd);
    } else if (strcmp(name, "export") == 0) {
        if (argc == 1) {
            extern char **environ;
            for (char **entry = environ; *entry; entry++)
                puts(*entry);
            return 0;
        }
        for (size_t i = 1; i < argc; i++) {
            char *equals = strchr(args[i], '=');
            if (equals) {
                size_t name_length = (size_t)(equals - args[i]);
                if (!name_length || !(isalpha((unsigned char)args[i][0]) || args[i][0] == '_')) {
                    fprintf(stderr, "forge: export: invalid variable name: %s\n", args[i]);
                    return 1;
                }
                for (size_t j = 1; j < name_length; j++) {
                    if (!(isalnum((unsigned char)args[i][j]) || args[i][j] == '_')) {
                        fprintf(stderr, "forge: export: invalid variable name: %s\n", args[i]);
                        return 1;
                    }
                }
                char *key = strndup(args[i], name_length);
                if (!key) {
                    perror("forge: strndup");
                    return 1;
                }
                int result = setenv(key, equals + 1, 1);
                free(key);
                if (result < 0) {
                    perror("forge: export");
                    return 1;
                }
            } else {
                if (!(isalpha((unsigned char)args[i][0]) || args[i][0] == '_')) {
                    fprintf(stderr, "forge: export: invalid variable name: %s\n", args[i]);
                    return 1;
                }
                for (size_t j = 1; args[i][j]; j++)
                    if (!(isalnum((unsigned char)args[i][j]) || args[i][j] == '_')) {
                        fprintf(stderr, "forge: export: invalid variable name: %s\n", args[i]);
                        return 1;
                    }
                if (setenv(args[i], getenv(args[i]) ? getenv(args[i]) : "", 1) < 0) {
                    perror("forge: export");
                    return 1;
                }
            }
        }
    } else if (strcmp(name, "exit") == 0) {
        if (argc > 2) {
            fprintf(stderr, "forge: exit: too many arguments\n");
            return 1;
        }
        if (argc == 2) {
            char *end;
            long code = strtol(args[1], &end, 10);
            if (*end) {
                fprintf(stderr, "forge: exit: numeric argument required\n");
                *should_exit = true;
                return 2;
            }
            last_status = (unsigned char)code;
        }
        *should_exit = true;
        return last_status;
    } else if (strcmp(name, "history") == 0) {
        for (size_t i = 0; i < history_count; i++)
            printf("%4zu  %s\n", i + 1, history[i]);
    } else if (strcmp(name, "jobs") == 0) {
        reap_jobs();
        for (size_t i = 0; i < job_count; i++)
            printf("[%d] %s %s\n", jobs[i].id, jobs[i].stopped ? "Stopped" : "Running", jobs[i].text);
    } else if (strcmp(name, "fg") == 0 || strcmp(name, "bg") == 0) {
        int id = parse_job_argument(argc > 1 ? args[1] : NULL);
        ssize_t index = find_job_by_id(id);
        if (index < 0) {
            fprintf(stderr, "forge: %s: no such job\n", name);
            return 1;
        }
        if (strcmp(name, "fg") == 0) {
            puts(jobs[index].text);
            return wait_for_job((size_t)index, true);
        }
        if (kill(-jobs[index].pgid, SIGCONT) < 0) {
            perror("forge: bg");
            return 1;
        }
        jobs[index].stopped = false;
        printf("[%d] %s &\n", jobs[index].id, jobs[index].text);
    } else if (strcmp(name, "help") == 0) {
        puts("forge: a small POSIX shell\n"
             "Built-ins: cd [dir], pwd, export [NAME=VALUE], exit [status], history,\n"
             "           jobs, fg [%job], bg [%job], help\n"
             "Supports pipelines, <, >, >>, 2>, &>, background jobs, quotes, escapes,\n"
             "and $NAME, ${NAME}, $?, and $$ expansion.");
    }
    return 0;
}
