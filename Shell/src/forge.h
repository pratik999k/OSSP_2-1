#ifndef FORGE_H
#define FORGE_H

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

enum token_kind {
    TOK_WORD,
    TOK_PIPE,
    TOK_IN,
    TOK_OUT,
    TOK_APPEND,
    TOK_ERR,
    TOK_BOTH,
    TOK_AMP
};

typedef struct {
    enum token_kind kind;
    char *text;
} Token;

typedef struct {
    char **argv;
    size_t argc;
    char *input;
    char *output;
    char *error;
    bool append;
    bool both;
} Command;

typedef struct {
    Command *commands;
    size_t count;
    bool background;
} Pipeline;

typedef struct {
    int id;
    pid_t pgid;
    size_t remaining;
    bool stopped;
    char *text;
} Job;

extern Job jobs[128];
extern size_t job_count;
extern int next_job_id;
extern char **history;
extern size_t history_count;
extern size_t history_capacity;
extern char *previous_directory;
extern int last_status;
extern int shell_interactive;
extern pid_t shell_pgid;
extern struct termios shell_modes;
extern volatile sig_atomic_t interrupted;

void *checked_realloc(void *ptr, size_t size);
char *copy_string(const char *s);
void append_char(char **buffer, size_t *length, size_t *capacity, char c);
void append_text(char **buffer, size_t *length, size_t *capacity, const char *text);

void add_history(const char *line);
char *read_line(void);
void print_line(const char *line);

int tokenize(const char *line, Token **result, size_t *result_count);
void free_tokens(Token *tokens, size_t count);
int parse_tokens(Token *tokens, size_t count, Pipeline *pipeline);
void free_pipeline(Pipeline *pipeline);
int open_redirections(const Command *command);

ssize_t find_job_by_id(int id);
void remove_job(size_t index);
void add_job(pid_t pgid, size_t remaining, bool stopped, const char *text);
void reap_jobs(void);
int parse_job_argument(const char *arg);
int wait_for_job(size_t index, bool foreground);

int builtin_index(const char *name);
int run_builtin(Command *command, bool *should_exit);

void child_signals_default(void);
void handle_sigint(int signal_number);
void initialize_shell(void);
int execute_pipeline(Pipeline *pipeline, const char *source, bool *should_exit);
void cleanup_shell(void);

#endif
