#include "forge.h"
char *pipeline_text(const Pipeline *pipeline) {
    size_t length = 1;
    for (size_t i = 0; i < pipeline->count; i++)
        for (size_t j = 0; j < pipeline->commands[i].argc; j++)
            length += strlen(pipeline->commands[i].argv[j]) + 1;
    char *text = calloc(length + pipeline->count * 3, 1);
    if (!text) {
        perror("forge: calloc");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < pipeline->count; i++) {
        if (i)
            strcat(text, " |");
        for (size_t j = 0; j < pipeline->commands[i].argc; j++) {
            strcat(text, j ? " " : " ");
            strcat(text, pipeline->commands[i].argv[j]);
        }
    }
    if (pipeline->background)
        strcat(text, " &");
    return text;
}

int execute_pipeline(Pipeline *pipeline, const char *source, bool *should_exit) {
    if (!pipeline->count)
        return 0;
    char *text = pipeline_text(pipeline);
    if (pipeline->count == 1 && builtin_index(pipeline->commands[0].argv[0]) >= 0 &&
        !pipeline->background) {
        Command *command = &pipeline->commands[0];
        int saved_in = -1, saved_out = -1, saved_err = -1;
        if (command->input)
            saved_in = dup(STDIN_FILENO);
        if (command->output || command->both)
            saved_out = dup(STDOUT_FILENO);
        if (command->error || command->both)
            saved_err = dup(STDERR_FILENO);
        int result = open_redirections(command);
        if (!result)
            result = run_builtin(command, should_exit);
        if (saved_in >= 0) { (void)dup2(saved_in, STDIN_FILENO); close(saved_in); }
        if (saved_out >= 0) { (void)dup2(saved_out, STDOUT_FILENO); close(saved_out); }
        if (saved_err >= 0) { (void)dup2(saved_err, STDERR_FILENO); close(saved_err); }
        free(text);
        return result;
    }

    size_t count = pipeline->count;
    int (*pipes)[2] = count > 1 ? calloc(count - 1, sizeof(*pipes)) : NULL;
    pid_t *pids = calloc(count, sizeof(*pids));
    if ((count > 1 && !pipes) || !pids) {
        perror("forge: calloc");
        free(pipes);
        free(pids);
        free(text);
        return 1;
    }
    for (size_t i = 0; i + 1 < count; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("forge: pipe");
            for (size_t j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            free(pipes);
            free(pids);
            free(text);
            return 1;
        }
    }

    pid_t pgid = 0;
    size_t launched = 0;
    for (size_t i = 0; i < count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("forge: fork");
            break;
        }
        if (pid == 0) {
            child_signals_default();
            if (!pgid)
                pgid = getpid();
            (void)setpgid(0, pgid);
            if (i > 0)
                (void)dup2(pipes[i - 1][0], STDIN_FILENO);
            if (i + 1 < count)
                (void)dup2(pipes[i][1], STDOUT_FILENO);
            for (size_t j = 0; j + 1 < count; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            if (open_redirections(&pipeline->commands[i]) < 0)
                _exit(1);
            if (builtin_index(pipeline->commands[i].argv[0]) >= 0) {
                bool child_exit = false;
                int result = run_builtin(&pipeline->commands[i], &child_exit);
                fflush(NULL);
                _exit(result);
            }
            execvp(pipeline->commands[i].argv[0], pipeline->commands[i].argv);
            fprintf(stderr, "forge: %s: %s\n", pipeline->commands[i].argv[0], strerror(errno));
            _exit(errno == ENOENT ? 127 : 126);
        }
        if (!pgid)
            pgid = pid;
        (void)setpgid(pid, pgid);
        pids[launched++] = pid;
    }
    for (size_t i = 0; i + 1 < count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    free(pipes);
    if (!launched) {
        free(pids);
        free(text);
        return 1;
    }
    if (launched < count) {
        (void)kill(-pgid, SIGTERM);
        for (size_t i = 0; i < launched; i++)
            (void)waitpid(pids[i], NULL, 0);
        free(pids);
        free(text);
        return 1;
    }
    free(pids);

    if (pipeline->background) {
        add_job(pgid, count, false, source && *source ? source : text);
        printf("[%d] %ld\n", jobs[job_count - 1].id, (long)pgid);
        free(text);
        return 0;
    }
    if (shell_interactive)
        (void)tcsetpgrp(STDIN_FILENO, pgid);
    int status = 0;
    size_t remaining = count;
    bool stopped = false;
    while (remaining) {
        int child_status;
        pid_t pid = waitpid(-pgid, &child_status, WUNTRACED);
        if (pid < 0) {
            if (errno == EINTR)
                continue;
            if (errno != ECHILD)
                perror("forge: waitpid");
            break;
        }
        if (WIFSTOPPED(child_status)) {
            stopped = true;
            status = 128 + WSTOPSIG(child_status);
            break;
        }
        if (WIFEXITED(child_status) || WIFSIGNALED(child_status)) {
            remaining--;
            status = WIFEXITED(child_status) ? WEXITSTATUS(child_status) :
                     128 + WTERMSIG(child_status);
        }
    }
    if (shell_interactive) {
        (void)tcsetpgrp(STDIN_FILENO, shell_pgid);
        (void)tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_modes);
    }
    if (stopped)
        add_job(pgid, remaining, true, source && *source ? source : text);
    free(text);
    return status;
}
