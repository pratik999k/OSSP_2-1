#include "forge.h"
void child_signals_default(void) {
    struct sigaction action = {0};
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    (void)sigaction(SIGINT, &action, NULL);
    (void)sigaction(SIGTSTP, &action, NULL);
    (void)sigaction(SIGQUIT, &action, NULL);
    (void)sigaction(SIGTTIN, &action, NULL);
    (void)sigaction(SIGTTOU, &action, NULL);
    (void)sigaction(SIGCHLD, &action, NULL);
}
void initialize_shell(void) {
    shell_interactive = isatty(STDIN_FILENO);
    if (shell_interactive) {
        while (tcgetpgrp(STDIN_FILENO) != (shell_pgid = getpgrp()))
            (void)kill(-shell_pgid, SIGTTIN);
        struct sigaction ignore = {0};
        ignore.sa_handler = SIG_IGN;
        sigemptyset(&ignore.sa_mask);
        (void)sigaction(SIGQUIT, &ignore, NULL);
        (void)sigaction(SIGTSTP, &ignore, NULL);
        (void)sigaction(SIGTTIN, &ignore, NULL);
        (void)sigaction(SIGTTOU, &ignore, NULL);
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0 && errno != EACCES && errno != EPERM)
            perror("forge: setpgid");
        if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
            perror("forge: tcsetpgrp");
        if (tcgetattr(STDIN_FILENO, &shell_modes) < 0)
            perror("forge: tcgetattr");
    } else {
        shell_pgid = getpgrp();
    }
    struct sigaction action = {0};
    action.sa_handler = handle_sigint;
    sigemptyset(&action.sa_mask);
    (void)sigaction(SIGINT, &action, NULL);
}
