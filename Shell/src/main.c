#include "forge.h"
int main(void) {
    initialize_shell();
    bool should_exit = false;
    while (!should_exit) {
        reap_jobs();
        if (shell_interactive) {
            char *cwd = getcwd(NULL, 0);
            printf("%s$ ", cwd ? cwd : "forge");
            free(cwd);
            fflush(stdout);
        }
        char *line = read_line();
        if (interrupted) {
            interrupted = 0;
            print_line("\n");
            free(line);
            last_status = 130;
            continue;
        }
        if (!line)
            break;
        add_history(line);
        Token *tokens = NULL;
        size_t token_count = 0;
        Pipeline pipeline = {0};
        if (tokenize(line, &tokens, &token_count) == 0) {
            if (parse_tokens(tokens, token_count, &pipeline) == 0)
                last_status = execute_pipeline(&pipeline, line, &should_exit);
        } else {
            last_status = 2;
        }
        free_pipeline(&pipeline);
        free_tokens(tokens, token_count);
        free(line);
    }
    cleanup_shell();
    return last_status;
}
