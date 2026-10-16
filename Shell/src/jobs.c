#include "forge.h"
ssize_t find_job_by_id(int id) {
    for (size_t i = 0; i < job_count; i++)
        if (jobs[i].id == id)
            return (ssize_t)i;
    return -1;
}

void remove_job(size_t index) {
    free(jobs[index].text);
    for (size_t i = index + 1; i < job_count; i++)
        jobs[i - 1] = jobs[i];
    job_count--;
}

void add_job(pid_t pgid, size_t remaining, bool stopped, const char *text) {
    if (job_count == sizeof(jobs) / sizeof(jobs[0])) {
        fprintf(stderr, "forge: job table full; process group %ld is untracked\n", (long)pgid);
        return;
    }
    jobs[job_count++] = (Job){next_job_id++, pgid, remaining, stopped, copy_string(text)};
}

void reap_jobs(void) {
    for (size_t i = 0; i < job_count;) {
        int status;
        pid_t pid;
        bool removed = false;
        while ((pid = waitpid(-jobs[i].pgid, &status,
                              WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                if (jobs[i].remaining)
                    jobs[i].remaining--;
                if (!jobs[i].remaining) {
                    printf("[%d] Done %s\n", jobs[i].id, jobs[i].text);
                    remove_job(i);
                    removed = true;
                    break;
                }
            } else if (WIFSTOPPED(status)) {
                jobs[i].stopped = true;
            } else if (WIFCONTINUED(status)) {
                jobs[i].stopped = false;
            }
        }
        if (!removed)
            i++;
    }
}
int parse_job_argument(const char *arg) {
    if (!arg || !*arg)
        return job_count ? jobs[job_count - 1].id : -1;
    if (*arg == '%')
        arg++;
    char *end;
    long id = strtol(arg, &end, 10);
    return *end || id <= 0 || id > 2147483647 ? -1 : (int)id;
}

int wait_for_job(size_t index, bool foreground) {
    Job *job = &jobs[index];
    job->stopped = false;
    if (foreground && shell_interactive)
        (void)tcsetpgrp(STDIN_FILENO, job->pgid);
    if (kill(-job->pgid, SIGCONT) < 0 && errno != ESRCH)
        perror("forge: SIGCONT");
    int status;
    while (job->remaining) {
        pid_t pid = waitpid(-job->pgid, &status, WUNTRACED);
        if (pid < 0) {
            if (errno == EINTR)
                continue;
            if (errno != ECHILD)
                perror("forge: waitpid");
            break;
        }
        if (WIFSTOPPED(status)) {
            job->stopped = true;
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status))
            job->remaining--;
    }
    if (foreground && shell_interactive) {
        (void)tcsetpgrp(STDIN_FILENO, shell_pgid);
        (void)tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_modes);
    }
    if (!job->remaining) {
        int result = WIFEXITED(status) ? WEXITSTATUS(status) :
                     WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 0;
        remove_job(index);
        return result;
    }
    if (job->stopped)
        printf("[%d] Stopped %s\n", job->id, job->text);
    return 128 + SIGTSTP;
}
