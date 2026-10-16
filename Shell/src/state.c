#include "forge.h"

Job jobs[128];
size_t job_count;
int next_job_id = 1;
char **history;
size_t history_count;
size_t history_capacity;
char *previous_directory;
int last_status;
int shell_interactive;
pid_t shell_pgid;
struct termios shell_modes;
volatile sig_atomic_t interrupted;

void cleanup_shell(void) {
    for (size_t i = 0; i < job_count; i++) {
        (void)kill(-jobs[i].pgid, SIGHUP);
        free(jobs[i].text);
    }
    for (size_t i = 0; i < history_count; i++)
        free(history[i]);
    free(history);
    free(previous_directory);
}
