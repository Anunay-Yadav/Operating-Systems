#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct command{
	char* name;
	char* args[100];
	int argc;
	int append;
	int fd[3];
	int error;
} command;

typedef struct command_pipe{
	command* cmd[100];
	int c;
} pipe_cmd;

void remove_end(char* s);
void parse(char* l,command* s);
pipe_cmd* parse_pipe(char* l);
int check(int id);
void initialise(command *s);
void close_fd(command* cur_cmd);
void handle_mult_pipe(pipe_cmd* curr,int** pipes);
void init_shell();
pipe_cmd* initialise_pipe();
void store_arg(command* cmd,char** arg_new,int count);
void reset(char* s);
void get_filename(command* cmd,int index,char* buffer);
void handle_input(command* cmd,int index);
void handle_output(command* cmd,int index);