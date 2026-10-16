#include "forge.h"
void expand_variable(const char *line, size_t *index, char **word, size_t *length,
                            size_t *capacity) {
    size_t i = *index + 1;
    char name[256];
    size_t n = 0;
    const char *value = "";

    if (line[i] == '?') {
        snprintf(name, sizeof(name), "%d", last_status);
        value = name;
        i++;
    } else if (line[i] == '$') {
        snprintf(name, sizeof(name), "%ld", (long)getpid());
        value = name;
        i++;
    } else {
        bool braced = line[i] == '{';
        if (braced)
            i++;
        if (!(isalpha((unsigned char)line[i]) || line[i] == '_')) {
            append_char(word, length, capacity, '$');
            *index += 1;
            return;
        }
        while (isalnum((unsigned char)line[i]) || line[i] == '_') {
            if (n + 1 < sizeof(name))
                name[n++] = line[i];
            i++;
        }
        if (braced && line[i] == '}')
            i++;
        name[n] = '\0';
        const char *found = getenv(name);
        if (found)
            value = found;
    }
    append_text(word, length, capacity, value);
    *index = i;
}

bool add_token(Token **tokens, size_t *count, size_t *capacity,
                      enum token_kind kind, const char *text) {
    if (*count == *capacity) {
        *capacity = *capacity ? *capacity * 2 : 16;
        *tokens = checked_realloc(*tokens, *capacity * sizeof(**tokens));
    }
    (*tokens)[*count].kind = kind;
    (*tokens)[*count].text = text ? copy_string(text) : NULL;
    (*count)++;
    return true;
}

void free_tokens(Token *tokens, size_t count) {
    for (size_t i = 0; i < count; i++)
        free(tokens[i].text);
    free(tokens);
}

int tokenize(const char *line, Token **result, size_t *result_count) {
    Token *tokens = NULL;
    size_t count = 0, token_capacity = 0;
    for (size_t i = 0; line[i];) {
        if (isspace((unsigned char)line[i])) {
            i++;
            continue;
        }
        if (line[i] == '2' && line[i + 1] == '>') {
            add_token(&tokens, &count, &token_capacity, TOK_ERR, NULL);
            i += 2;
            continue;
        }
        if (line[i] == '&' && line[i + 1] == '>') {
            add_token(&tokens, &count, &token_capacity, TOK_BOTH, NULL);
            i += 2;
            continue;
        }
        if (line[i] == '|' || line[i] == '<' || line[i] == '>' || line[i] == '&') {
            enum token_kind kind;
            if (line[i] == '|')
                kind = TOK_PIPE;
            else if (line[i] == '<')
                kind = TOK_IN;
            else if (line[i] == '&')
                kind = TOK_AMP;
            else if (line[i + 1] == '>') {
                kind = TOK_APPEND;
                i++;
            } else
                kind = TOK_OUT;
            add_token(&tokens, &count, &token_capacity, kind, NULL);
            i++;
            continue;
        }

        char *word = NULL;
        size_t length = 0, capacity = 0;
        bool started = false;
        char quote = '\0';
        while (line[i] && (quote || (!isspace((unsigned char)line[i]) &&
                                     line[i] != '|' && line[i] != '<' &&
                                     line[i] != '>' && line[i] != '&'))) {
            char c = line[i];
            if (quote == '\'') {
                if (c == '\'')
                    quote = '\0';
                else
                    append_char(&word, &length, &capacity, c);
                i++;
                started = true;
            } else if (c == '\\' && quote != '\'') {
                if (!line[i + 1]) {
                    fprintf(stderr, "forge: trailing escape\n");
                    free(word);
                    free_tokens(tokens, count);
                    return -1;
                }
                append_char(&word, &length, &capacity, line[i + 1]);
                i += 2;
                started = true;
            } else if (c == '\'' && quote != '"') {
                quote = quote ? '\0' : '\'';
                i++;
                started = true;
            } else if (c == '"' && quote != '\'') {
                quote = quote ? '\0' : '"';
                i++;
                started = true;
            } else if (c == '$' && quote != '\'') {
                expand_variable(line, &i, &word, &length, &capacity);
                started = true;
            } else {
                append_char(&word, &length, &capacity, c);
                i++;
                started = true;
            }
        }
        if (quote) {
            fprintf(stderr, "forge: unmatched quote\n");
            free(word);
            free_tokens(tokens, count);
            return -1;
        }
        if (started)
            add_token(&tokens, &count, &token_capacity, TOK_WORD, word ? word : "");
        free(word);
    }
    *result = tokens;
    *result_count = count;
    return 0;
}
