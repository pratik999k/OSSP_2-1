#include "forge.h"
void print_line(const char *line) {
    (void)write(STDOUT_FILENO, line, strlen(line));
}

void handle_sigint(int signal_number) {
    (void)signal_number;
    interrupted = 1;
}

char *read_interactive_line(void) {
    struct termios original, editing;
    if (tcgetattr(STDIN_FILENO, &original) < 0) {
        perror("forge: tcgetattr");
        return NULL;
    }
    editing = original;
    editing.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    editing.c_cc[VMIN] = 1;
    editing.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editing) < 0) {
        perror("forge: tcsetattr");
        return NULL;
    }

    char *line = NULL;
    size_t length = 0, capacity = 0, history_position = history_count;
    bool escape = false;
    bool submitted = false;
    for (;;) {
        unsigned char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n < 0 && errno == EINTR) {
            if (interrupted) {
                free(line);
                line = NULL;
                length = 0;
                break;
            }
            continue;
        }
        if (n <= 0) {
            if (n < 0)
                perror("forge: read");
            break;
        }
        if (escape) {
            if (c == '[') {
                escape = false;
                continue;
            }
            if (c == 'A' && history_position > 0) {
                history_position--;
                while (length)
                    print_line("\b \b"), length--;
                if (line)
                    line[0] = '\0';
                append_text(&line, &length, &capacity, history[history_position]);
                print_line(line);
            } else if (c == 'B' && history_position < history_count) {
                history_position++;
                while (length)
                    print_line("\b \b"), length--;
                if (line)
                    line[0] = '\0';
                if (history_position < history_count)
                    append_text(&line, &length, &capacity, history[history_position]);
                print_line(line ? line : "");
            }
            escape = false;
            continue;
        }
        if (c == '\033') {
            escape = true;
        } else         if (c == '\n' || c == '\r') {
            print_line("\n");
            submitted = true;
            break;
        } else if (c == 4 && length == 0) {
            free(line);
            line = NULL;
            break;
        } else if (c == 127 || c == 8) {
            if (length) {
                length--;
                line[length] = '\0';
                print_line("\b \b");
            }
        } else if (isprint(c) || c == '\t') {
            append_char(&line, &length, &capacity, (char)c);
            (void)write(STDOUT_FILENO, &c, 1);
            history_position = history_count;
        }
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original) < 0) {
        perror("forge: restore terminal");
        free(line);
        return NULL;
    }
    return submitted && !line ? copy_string("") : line;
}

char *read_line(void) {
    if (!shell_interactive) {
        char *line = NULL;
        size_t capacity = 0;
        ssize_t n = getline(&line, &capacity, stdin);
        if (n < 0) {
            free(line);
            return NULL;
        }
        if (n && line[n - 1] == '\n')
            line[n - 1] = '\0';
        return line;
    }
    return read_interactive_line();
}
