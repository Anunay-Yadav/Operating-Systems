#include "shell.h"
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

//removes new line
void remove_end(char* s){
	for (int i = 0; i < 100; ++i)
	{
		if(s[i] == '\n')
			s[i] = '\0';
	}
}

//initialises command structure
command* initialise_cmd(){
	command* ret = (command*)malloc(sizeof(command));
	ret -> fd[0] = -1;
	ret -> fd[1] = -1;
	ret -> append = 0;
	ret -> error = 0;
	ret -> fd[2] = -1;
	return ret;
}
//stores argument from buffer to command structure
void store_arg(command* cmd,char** arg_new,int count){
	
	for (int i = 0; i < cmd -> argc; ++i)
	{
		cmd -> args[i] = NULL;
	}

	cmd -> argc = count;

	for (int i = 0; i < count; ++i)
	{
		cmd -> args[i] = arg_new[i];
	}
}
//handles output redirection
void handle_output(command* cmd,int index){
	
	if(cmd -> fd[1] != -1) {
		close(cmd -> fd[1]);
	}

	if(strlen(cmd -> args[index]) == 2 && cmd -> args[index][1] == '>'){ // appending to a file
		if(access(cmd -> args[index + 1],F_OK) != -1){
			cmd -> fd[1] = open(cmd -> args[index + 1],O_APPEND | O_WRONLY,0666);
		}
		else cmd -> fd[1] = creat(cmd -> args[index + 1],0666); 
		cmd -> append = 1;
	}
	else{ //creating a new file or replacing the contents of previous file
		cmd -> fd[1] = creat(cmd -> args[index + 1],0666);
		cmd -> append = 0;
	}
}

//handles input redirection
void handle_input(command* cmd,int index){
	if(cmd -> fd[0] != -1) close(cmd -> fd[0]);
	cmd -> fd[0] = open(cmd -> args[index + 1],O_RDWR);
}

//gets filename and stores them on buffer
void get_filename(command* cmd,int index,char* buffer){
	//parsing the file name
	
	for (int j = 2; j < strlen(cmd -> args[index]); ++j)
	{
		buffer[j-2] = cmd -> args[index][j];
	}

}
//resets buffer back to null characters
void reset(char* s){
	for (int i = 0; i < 100; ++i)
	{
		s[i] = '\0';
	}
}
void parse(char* cmd_line,command* cmd){
	// creates token on the basis spaces
	char* token = strtok(cmd_line," ");
	remove_end(token);
	cmd -> name = token;
	//filling command with appropriate command arguments and tokens
	while(token != NULL){
		cmd -> args[cmd -> argc] = token;
		cmd -> argc++;
		token = strtok(NULL," ");
		if(token != NULL) remove_end(token);
	}
	
	//buffer to hold arguments
	char* arg_new[100];
	char filename[100];
	int count = 0;

	for (int index = 0; index < cmd -> argc; ++index)
	{
		if(cmd -> args[index] == NULL) continue;

		//case 1 : handling : < filename
		if(cmd -> args[index][0] == '>'){
			//check
			if(cmd -> args[index + 1] == NULL) {
				cmd -> error = 1;
				break;
			}
			//handling file descriptors
			handle_output(cmd,index);
			index++;
		}
		else if(cmd -> args[index][0] == '<'){ //case 2: handling : > filename
			//check
			if(cmd -> args[index + 1] == NULL || access(cmd -> args[index + 1],F_OK) == -1) {
				cmd -> error = 1;
				break;
			}
			//handling file descriptors
			handle_input(cmd,index);
			index++;
		}
		else if(strlen(cmd -> args[index]) >= 2 && cmd -> args[index][0] == '1' && cmd -> args[index][1] == '>'){ //case 3 : >filename
			//closing output fd
			if(cmd -> fd[1] != -1) close(cmd -> fd[1]);
			//parsing the file name
			char filename[100];
			get_filename(cmd,index,filename);
			
			//handling fd's
			cmd -> fd[1] = creat(filename,0666);
			cmd -> append = 0;
			index++;
		}
		else if(strlen(cmd -> args[index]) == 4 && cmd -> args[index][0] == '2' && cmd -> args[index][1] == '>' && cmd -> args[index][2] == '&' && cmd -> args[index][3] == '1'){ //case : 2>&1
			cmd -> fd[2] = 1;
			index++;
		}
		else if(strlen(cmd -> args[index]) >= 2 && cmd -> args[index][0] == '2' && cmd -> args[index][1] == '>'){ //case : 2>filename
			if(cmd -> fd[2] != -1) close(cmd -> fd[2]);
			//parsing filename
			get_filename(cmd,index,filename);

			//handling fd's
			cmd -> fd[2] = creat(filename,0666);
			index++;
		}
		else{
			arg_new[count] = cmd -> args[index];
			count++;
		}
		//clear filename buffer
		reset(filename);
	}

	//store parsed arguments form buffer to structure
	store_arg(cmd,arg_new,count);

}

//starts parsing new command
command* parse_new_cmd(char* cmd){
	command* ret = initialise_cmd();
	parse(cmd,ret);
	return ret;
}

//initialises pipe structure
pipe_cmd* initialise_pipe(){
	pipe_cmd* ret = (pipe_cmd*) malloc(sizeof(pipe_cmd));
	ret -> c = 0;
	return ret;
}
pipe_cmd* parse_pipe(char* cmd){
	
	pipe_cmd* ret = initialise_pipe();

	char* buffer_pipe[100];
	
	//create tokens on the basis of | because of pipes
	remove_end(cmd);
	char* token = strtok(cmd,"|");
	int ind = 0;
	remove_end(token);
	buffer_pipe[ind++] = token;
	
	//store all the tokens on buffer_pipe
	while(token != NULL)
	{
		token = strtok(NULL,"|");
		if(token != NULL) {
			remove_end(token);
			buffer_pipe[ind++] = token;
		}
	}

	//create command of each cmd enclosed withing command and store
	for (int j = 0; j < ind; ++j)
	{
		ret -> cmd[j] = parse_new_cmd(buffer_pipe[j]);
	}
	ret -> c = ind;

	return ret;
}

//check wether id is standard or not
int check(int id){
	return !(id == 1 || id == 2 || id == 0);
}

//gets the current directory and os name
void init_shell(){
	char cwd[100];
	char user_name[100];
	getlogin_r(user_name,sizeof(user_name));
	printf("shell@%s:",user_name);
	getcwd(cwd,sizeof(cwd));
	printf("%s$ ",cwd);
}

//handling multiple pipe redirections
void handle_mult_pipe(pipe_cmd* curr,int** pipes){
	curr -> cmd[0] -> fd[0] = 0;
	for (int i = 1; i < curr -> c; ++i)
	{
		curr -> cmd[i-1] -> fd[1] = pipes[i-1][1];
		curr -> cmd[i] -> fd[0] =  pipes[i-1][0];
	}
	curr -> cmd[curr -> c - 1] -> fd[1] = 1;
}

//close file descriptors
void close_fd(command* cur_cmd){
	if(cur_cmd -> fd[0] != -1 && check(cur_cmd -> fd[0])) close(cur_cmd -> fd[0]);
	if(cur_cmd -> fd[1] != -1 && check(cur_cmd -> fd[1])) close(cur_cmd -> fd[1]);	
	if(cur_cmd -> fd[2] != -1 && check(cur_cmd -> fd[2])) close(cur_cmd -> fd[2]);
}

int main(){

	char cur_cmd_str[1000];
	int pid;

	while(1){

		//print the starting layout
		init_shell();

		//get the command from the terminal to be parses
		fgets(cur_cmd_str,sizeof(cur_cmd_str),stdin);
		remove_end(cur_cmd_str);
		pipe_cmd* curr = parse_pipe(cur_cmd_str);

		//initialising pipes
		int** pipes;
		pipes = (int**) malloc(sizeof(int*)*(curr -> c + 1));
		for (int i = 0; i < curr -> c; ++i)
		{
			pipes[i] = (int*)malloc(sizeof(int)*2);
			pipe(pipes[i]);
		}

		//multiple pipes handled
		handle_mult_pipe(curr,pipes);
 
		for (int pipe_ind = 0; pipe_ind < curr -> c; ++pipe_ind)
		{
			//command enclosed withing current pipe index
			command* cur_cmd = curr -> cmd[pipe_ind];

			//checks
			if(cur_cmd == NULL)
				continue;
			
			if(cur_cmd -> error){
				printf("ERROR\n");
				continue;
			}
			else if(strcmp("cd",cur_cmd -> name) == 0 && curr -> c == 1){
				//cd command handled
				chdir(cur_cmd -> args[1]);
				continue;		
			}
			else if(strcmp("exit",cur_cmd -> name) == 0)
				return 0;

			//creating child process
			pid = fork();
			if(pid == 0){
				//child process
				//handle io redirection
				if(cur_cmd -> fd[0] != -1) {
					dup2(cur_cmd -> fd[0],0);
				}
				if(cur_cmd -> fd[1] != -1){
					dup2(cur_cmd -> fd[1],1);
				}
				if(cur_cmd -> fd[2] != -1) {
					dup2(cur_cmd -> fd[2],2); 
				}

				//execute command with execvp
				int c = execvp(cur_cmd -> name,cur_cmd -> args);

				//check
				if(c < 0)
					printf("INVALID COMMAND\n");
			}
			else wait(NULL);

			//close opened file descriptors
			close_fd(cur_cmd);
		}
	}
}