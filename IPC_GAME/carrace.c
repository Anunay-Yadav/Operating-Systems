#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
// you can change the prototype of existing
// functions, add new routines, and global variables
// cheatmode, car1, car2, report are different processes
// they communicate with each other via pipes
int car_cheat_pip[3][2],car_rep_pip[3][2],rep_car_pip[3][2];
char emp = '\0';
// step-1
// ask user's if they want to cheat
// if yes, ask them if they want to relocate car1 or car2
// ask them the new location of car1/car2
// instruct car1/car2 to relocate to new position
// goto step-1
int isdec(char k){
	// int l = (int k) - 48;
	switch(k){
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': return 1;
		default : return 0;

	}
	return 0;
}
int to_num(char* k){ // converts string to number
	int s = 0;
	for (int i = 0; i < 10; ++i)
	{
		if(!isdec(k[i])){
			s = i-1;
			break;
		}
	}
	int ans = 0;
	int prod = 1;
	for (int i = s; i >= 0; i--)
	{
		ans += ((int) k[i] - 48)*prod;
		prod *= 10;
	}
	return ans;
}
char* tostring(int n){
	char * s= (void*) malloc(sizeof(char)*100);
	int k = 1;
	int l = 0;
	while(k <= n){
		l++;
		k *= 10;
	}
	l--;
	k /= 10;
	for (int i = l; i >= 0; i--)
	{
		s[i] = n%10 + 48;
		n /= 10;
	}
	return s;
}
void print(char* k){
	int e = strlen(k);
	write(1,k,e);
}
void refresh(char* k){
	for (int i = 0; i < 100; ++i)
	{
		k[i] = emp;
	}
	k[99] = '\0';
}
void cheatmode()
{	
	char buffer[100];
	fcntl(0, F_SETFL, O_NONBLOCK);
	while(1){
		print("cheatmode : Do you want to cheat :) (Y,N)? \n");
		char k;
		int m = read(0,buffer,100);
		if(m != -1){
			k = buffer[0];
			refresh(buffer);
			if(k == 'Y' || k == 'y'){
				print("enter the new location with the car number ex(1 2) 1 : new loc and 2 : carnumber\n");
				int l,y;int o = read(0,buffer,100);
				while(o == -1) {
					sleep(1);
					o = read(0,buffer,100);
				}
				if(o != -1){
					char *e = malloc(sizeof(char)*100);
					int i = 0;
					for (i = 0; i < 100; ++i)
					{
						if(buffer[i] == ' ' || buffer[i] == '\0')
							{	e[i] = '\n';
								break;
							}
						else
							e[i] = buffer[i];
					}
					l = to_num(e);
					refresh(e);
					i++;
					int e1 = i;
					for (; i < 100; ++i)
					{
						if(buffer[i] == ' ' || buffer[i] == '\0')
							break;
						else
							e[i - e1] = buffer[i];
					}
					y = to_num(e);
					refresh(buffer);
					sprintf(buffer,"%d",l);
					write(car_cheat_pip[y][1],buffer,strlen(buffer));// writing to corresponging car to get to new location
					refresh(buffer);
				}
			}
		}
		sleep(3);
	}
}

// step-1
// check if report wants me to terminate
// if yes, terminate
// sleep for a second
// generate a random number r between 0-10
// advance the current position by r steps
// check if cheat mode relocated me
// if yes set the current position to the new position
// send the current postion to report
// make sure that car1 and car2 generates a different
// random number
// goto step-1

void car1()
{
	int pos = 0;
	srand(getpid());
	fcntl(rep_car_pip[1][0], F_SETFL, O_NONBLOCK); // no blocking created
	fcntl(car_cheat_pip[1][0], F_SETFL, O_NONBLOCK);
	char buffer[100];
	while(1){
		int k = read(rep_car_pip[1][0],buffer,10);
		if(k != -1){
			exit(0);// if report wants car1 process to exit than exit is called
		}
		refresh(buffer);
		sleep(1);
		k = rand()%11;
		pos += k;
		k = read(car_cheat_pip[1][0],buffer,10); // read from cheat mode
		if(k != -1){
			k = to_num(buffer);
			pos = k;
		}
		sprintf(buffer,"%d",pos);
		int e2 = strlen(buffer);
		write(car_rep_pip[1][1],buffer,e2);// write to report to print status
		refresh(buffer);
	}
}

// step-1
// check if report wants me to terminate
// if yes, terminate
// sleep for a second
// generate a random number r between 0-10
// advance the current position by r steps
// check if cheat mode relocated me
// if yes set the current position to the new position
// send the current postion to report
// make sure that car1 and car2 generates a different
// random number
// goto step-1
void car2()
{
	int pos = 0;
	srand(getpid());
	fcntl(rep_car_pip[2][0], F_SETFL, O_NONBLOCK);
	fcntl(car_cheat_pip[2][0], F_SETFL, O_NONBLOCK);
	char buffer[100];
	while(1){
		int k = read(rep_car_pip[2][0],buffer,10);
		if(k != -1){
			exit(0);// exit if report wants it to
		}
		refresh(buffer);
		sleep(1);
		k = rand()%11;
		pos += k;
		k = read(car_cheat_pip[2][0],buffer,10);
		if(k != -1){
			k = to_num(buffer);
			pos = k;
		}
		sprintf(buffer,"%d",pos);
		int e2 = strlen(buffer);
		write(car_rep_pip[2][1],buffer,e2);
		refresh(buffer);
	}

}

// step-1
// sleep for a second
// read the status of car1
// read the status of car2
// whoever completes a distance of 100 steps is decalared as winner
// if both cars complete 100 steps together then the match is tied
// if (any of the cars have completed 100 steps)
//    print the result of the match
//    ask cars to terminate themselves
//    kill cheatmode using kill system call
//    return to main if report is the main process
// else
// 	  print the status of car1 and car2
// goto step-1
void report(int pid)
{
	char buffer[100];
	while(1){
		sleep(1);
		read(car_rep_pip[1][0],buffer,10); // read from car1 with block
		int k = to_num(buffer);
		refresh(buffer);
		read(car_rep_pip[2][0],buffer,10);	// read from car2 with block
		int e = to_num(buffer);
		refresh(buffer);
		if(e >= 100 || k >= 100){
			if(e >= 100 && k >= 100)
				print("tie both of them reached the end\n");
			else if(e >= 100){
				print("car 2 won by reaching 100 steps first\n");
			}
			else{
				print("car 1 won by reaching 100 steps first\n");
			}
			sprintf(buffer,"%d",e);
			write(rep_car_pip[2][1],buffer,strlen(buffer));
			write(rep_car_pip[1][1],buffer,strlen(buffer));
			kill(pid,SIGQUIT);
			return;
		}
		else{
			print("car1 : ");
			print(tostring(k));
			print(" car2 : ");
			print(tostring(e));
			print("\n");
		}
	}
}

// create pipes
// create all processes
// wait for all processes to terminate
int main()
{
	for (int i = 0; i < 2; ++i)
	{
		pipe(car_rep_pip[i + 1]);
		pipe(car_cheat_pip[i + 1]);
		pipe(rep_car_pip[i + 1]);
	}
	int race1id = fork();
	if(race1id == 0){
		car1();
	}
	else{
		int race2id = fork();
		if(race2id == 0){
			car2();
		}	
		else{
			int ch = fork();
			if(ch == 0){
				cheatmode();
			}
			else{
				report(ch);
				wait(NULL);
				wait(NULL);
				wait(NULL);
			}
		}
	}
	return 0;
}
