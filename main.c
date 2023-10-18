#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHELL_PROMPT "🦑 $> "
#define TOKEN_BUFFERSIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a"
#define READLINE_BUFFERSIZE 1024

void ink_loop(void);
char *read_line(void);
char **split_line(char *line);
int launch(char **args);
int num_builtins();
int execute(char **args);

int main(int argc, char **argv) {
  ink_loop();

  return EXIT_SUCCESS;
}

void ink_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf(SHELL_PROMPT);

    line = read_line();
    args = split_line(line);
    status = execute(args);

    free(line);
    free(args);

  } while (status);
}

char *read_line(void) {
  int bufsize = READLINE_BUFFERSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);

  int c;

  if (!buffer) {
    fprintf(stderr, "ink: alloaction error\n");
  }

  while (1) {
    c = getchar();

    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position += 1;

    if (position >= bufsize) {
      bufsize += READLINE_BUFFERSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "ink: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

char **split_line(char *line) {
  int bufsize = TOKEN_BUFFERSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token;

  if (!tokens) {
    fprintf(stderr, "ink: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TOKEN_DELIMITERS);
  while (token != NULL) {
    tokens[position] = token;
    position += 1;

    if (position >= bufsize) {
      bufsize += TOKEN_BUFFERSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));

      if (!tokens) {
        fprintf(stderr, "ink: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOKEN_DELIMITERS);
  }
  tokens[position] = NULL;
  return tokens;
}

// HANDLES LAUNCHING PROGRAMS
int launch(char **args) {
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // we are in a child process
    if (execvp(args[0], args) == -1) {
      perror("inkshell");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // something went wrong forking
    perror("inkshell");
  } else {
    // we are in the parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

// BUILT IN COMMANDS
int ink_cd(char **args);
int ink_help(char **args);
int ink_exit(char **args);

char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[])(char **) = {&ink_cd, &ink_help, &ink_exit};

int num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

int ink_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "inkshell: expected argument to cd");
  } else {
    if (chdir(args[1]) != 0) {
      perror("inkshell");
    }
  }

  return 1;
}

int ink_help(char **args) {
  int i;
  printf("Inkshell\n");
  printf("Like bash but bad\n");
  printf("It contains the following built in commands:\n");

  for (i = 0; i < num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int ink_exit(char **args) { return 0; }

int execute(char **args) {
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return launch(args);
}
