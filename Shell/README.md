# forge

`forge` is a small POSIX shell written in C for the operating-systems and
systems-programming exercises in `os.docx`. It builds with the system C
compiler on macOS and Linux and can also read commands from standard input.

## Build and run

```sh
make
./forge
```

Run the automated smoke tests with `make test`; remove the executable with
`make clean`.

## Source layout

The shell is split into focused C modules under `src/`: `main.c` handles the
input loop, `input.c` and `history.c` manage interactive input, `lexer.c` and
`parser.c` process commands, `executor.c` and `redirection.c` launch them,
`builtins.c` and `jobs.c` implement shell commands and job control,
`signals.c` handles terminal signals, and `memory.c` and `state.c` provide
shared utilities and state. `forge.h` defines the shared data types and APIs.

## Implemented features

- Interactive prompt, line editing, backspace, Ctrl-D, and up/down command
  history navigation. The `history` built-in lists commands for the current
  session.
- Tokenization with whitespace handling, single/double quotes, backslash
  escapes, and `$NAME`, `${NAME}`, `$?`, and `$$` expansion.
- PATH-based command execution with `fork`, `execvp`, process groups, and
  `waitpid`.
- Pipelines and `<`, `>`, `>>`, `2>`, and `&>` redirections.
- `cd` (including `cd -`), `pwd`, `export`, `exit`, `history`, `jobs`, `fg`,
  `bg`, and `help` built-ins.
- Background job tracking, terminal handoff for foreground jobs, and
  interrupt/stop signal handling.

This is an educational shell, not a complete POSIX shell: shell functions,
control-flow syntax, globbing, command substitution, and heredocs are not
implemented.
