#include "forge.h"
void add_history(const char *line) {
    if (!*line)
        return;
    if (history_count && strcmp(history[history_count - 1], line) == 0)
        return;
    if (history_count == history_capacity) {
        history_capacity = history_capacity ? history_capacity * 2 : 32;
        history = checked_realloc(history, history_capacity * sizeof(*history));
    }
    history[history_count++] = copy_string(line);
}
