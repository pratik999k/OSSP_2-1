#include "forge.h"
void *checked_realloc(void *ptr, size_t size) {
    void *result = realloc(ptr, size);
    if (!result && size) {
        perror("forge: realloc");
        exit(EXIT_FAILURE);
    }
    return result;
}

char *copy_string(const char *s) {
    char *copy = strdup(s);
    if (!copy) {
        perror("forge: strdup");
        exit(EXIT_FAILURE);
    }
    return copy;
}

void append_char(char **buffer, size_t *length, size_t *capacity, char c) {
    if (*length + 1 >= *capacity) {
        *capacity = *capacity ? *capacity * 2 : 32;
        *buffer = checked_realloc(*buffer, *capacity);
    }
    (*buffer)[(*length)++] = c;
    (*buffer)[*length] = '\0';
}

void append_text(char **buffer, size_t *length, size_t *capacity, const char *text) {
    while (*text)
        append_char(buffer, length, capacity, *text++);
}
