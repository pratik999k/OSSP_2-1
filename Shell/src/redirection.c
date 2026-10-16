#include "forge.h"
int open_redirections(const Command *command) {
    if (command->input) {
        int fd = open(command->input, O_RDONLY);
        if (fd < 0) {
            perror(command->input);
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("forge: dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }
    if (command->output) {
        int flags = O_WRONLY | O_CREAT | (command->append ? O_APPEND : O_TRUNC);
        int fd = open(command->output, flags, 0666);
        if (fd < 0) {
            perror(command->output);
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) < 0 ||
            (command->both && dup2(fd, STDERR_FILENO) < 0)) {
            perror("forge: dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }
    if (command->error) {
        int fd = open(command->error, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            perror(command->error);
            return -1;
        }
        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("forge: dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }
    return 0;
}
