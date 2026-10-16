#include "forge.h"
void free_pipeline(Pipeline *pipeline) {
    for (size_t i = 0; i < pipeline->count; i++) {
        Command *command = &pipeline->commands[i];
        for (size_t j = 0; j < command->argc; j++)
            free(command->argv[j]);
        free(command->argv);
        free(command->input);
        free(command->output);
        free(command->error);
    }
    free(pipeline->commands);
    memset(pipeline, 0, sizeof(*pipeline));
}

int parse_tokens(Token *tokens, size_t count, Pipeline *pipeline) {
    memset(pipeline, 0, sizeof(*pipeline));
    if (!count)
        return 0;
    pipeline->commands = calloc(count + 1, sizeof(*pipeline->commands));
    if (!pipeline->commands) {
        perror("forge: calloc");
        return -1;
    }
    pipeline->count = 1;
    for (size_t i = 0; i < count; i++) {
        Command *command = &pipeline->commands[pipeline->count - 1];
        if (tokens[i].kind == TOK_AMP) {
            if (i != count - 1 || pipeline->background) {
                fprintf(stderr, "forge: '&' must appear at the end of a command\n");
                goto invalid;
            }
            pipeline->background = true;
            continue;
        }
        if (tokens[i].kind == TOK_PIPE) {
            if (!command->argc) {
                fprintf(stderr, "forge: missing command before '|'\n");
                goto invalid;
            }
            pipeline->count++;
            continue;
        }
        if (tokens[i].kind == TOK_WORD) {
            command->argv = checked_realloc(command->argv, (command->argc + 2) * sizeof(char *));
            command->argv[command->argc++] = copy_string(tokens[i].text);
            command->argv[command->argc] = NULL;
            continue;
        }
        if (i + 1 >= count || tokens[i + 1].kind != TOK_WORD) {
            fprintf(stderr, "forge: redirection requires a filename\n");
            goto invalid;
        }
        char **target = NULL;
        switch (tokens[i].kind) {
        case TOK_IN: target = &command->input; break;
        case TOK_OUT: target = &command->output; break;
        case TOK_APPEND:
            target = &command->output;
            command->append = true;
            break;
        case TOK_ERR: target = &command->error; break;
        case TOK_BOTH:
            target = &command->output;
            command->both = true;
            break;
        default: goto invalid;
        }
        free(*target);
        *target = copy_string(tokens[++i].text);
    }
    if (!pipeline->commands[pipeline->count - 1].argc) {
        fprintf(stderr, "forge: missing command after '|'\n");
        goto invalid;
    }
    return 0;

invalid:
    free_pipeline(pipeline);
    return -1;
}
