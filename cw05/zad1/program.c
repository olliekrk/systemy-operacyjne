//
// Created by olliekrk on 05.04.19.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define ARGS_REQUIRED 2
#define MAX_LINES 10
#define MAX_COMMANDS 10
#define MAX_ARGS 10

typedef struct {
    char **program_args;
} command;

typedef struct {
    command *commands;
    int no_commands;
} command_line;

char *read_file_content(char *);

command_line *parse_command_lines(char *, int *);

command_line parse_single_command_line(char *);

command parse_single_command(char *);

void execute_command_line(command_line);

void show_error(char *);

int main(int argc, char **argv) {
    if (argc != ARGS_REQUIRED)
        show_error("Invalid number of arguments");

    char *file_content = read_file_content(argv[1]);

    int no_lines = 0;
    command_line *lines = parse_command_lines(file_content, &no_lines);
    command_line line;
    int counter;

    for (int i = 0; i < no_lines; i++) {
        counter = 0;
        line = lines[i];

        if (line.commands == NULL)
            break;

        execute_command_line(line);
        while (counter < line.no_commands) {
            counter++;
            wait(NULL);
        }
    }

    free(lines);
    free(file_content);
    return 0;
}

char *read_file_content(char *f_name) {
    FILE *file = fopen(f_name, "r");
    if (!file) show_error("Failed to open given file");

    fseek(file, 0, SEEK_END);
    size_t f_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(f_size + 1);
    if (!content) show_error("Failed to allocate memory for buffer");

    fread(content, sizeof(char), f_size, file);
    fclose(file);

    return content;
}

//sets number of parsed lines to variable 'no_lines'
command_line *parse_command_lines(char *content, int *no_lines) {
    char *tmp_ptr;
    command_line *cmd_lines = calloc(MAX_LINES + 1, sizeof(command_line *));
    int i = 0;
    char *single_line = strtok_r(content, "\n", &tmp_ptr);
    while (single_line != NULL) {
        cmd_lines[i] = parse_single_command_line(single_line);
        i++;
        single_line = strtok_r(NULL, "\n", &tmp_ptr);
    }

    *no_lines = i;
    return cmd_lines;
}

command_line parse_single_command_line(char *line) {
    char *tmp_ptr;
    command *commands = calloc(MAX_COMMANDS + 1, sizeof(command));
    int i = 0;
    char *single_command = strtok_r(line, "|", &tmp_ptr);
    while (single_command != NULL) {
        commands[i] = parse_single_command(single_command);
        i++;
        single_command = strtok_r(NULL, "|", &tmp_ptr);
    }

    command_line cmd_line;
    cmd_line.commands = commands;
    cmd_line.no_commands = i;
    return cmd_line;
}

command parse_single_command(char *single_command) {
    char *tmp_ptr;
    char **args = calloc(MAX_ARGS + 1, sizeof(char *));
    command cmd;

    int i = 0;
    char *single_arg = strtok_r(single_command, " ", &tmp_ptr);

    while (single_arg != NULL) {
        args[i++] = single_arg;
        single_arg = strtok_r(NULL, " ", &tmp_ptr);
    }
    cmd.program_args = args;
    return cmd;
}

void execute_command_line(command_line cmd_line) {
    //fd[0] -> end for output
    //fd[1] -> end for input
    //dup2(fd[1], STDIN_FILENO) -> means that we replace STDIN with our pipe input end

    int A[2];
    int B[2];
    for (int i = 0; i < cmd_line.no_commands; i++) {
        A[0] = B[0];
        pipe(B);
        pid_t child = fork();
        if (child == 0) {
            close(B[0]);

            if (i != 0)
                dup2(A[0], STDIN_FILENO);

            if (i < cmd_line.no_commands - 1)
                dup2(B[1], STDOUT_FILENO);

            execvp(cmd_line.commands[i].program_args[0], cmd_line.commands[i].program_args);
            exit(EXIT_SUCCESS);
        }
        close(A[0]);
        close(B[1]);
        wait(NULL);
    }
}

void show_error(char *msg) {
    perror(msg);
    exit(1);
}
