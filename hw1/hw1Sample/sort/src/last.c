#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>

void createpipe(int fd[][2],int N){
	for(int i=0;i<N;i++){
		pipe(fd[i]);
	}	
}
void closeall(int fd[][2],int N){
	for(int i =0;i<N;i++){
		close(fd[i][1]);
		close(fd[i][0]);
	}
}
void getlines(int fd[][2],int N){
	char input[512];
	for(int i=0;fgets(input,512,stdin);i++){
		write(fd[(i%N)][1],input,strlen(input));
	}
}

int main(int argc, char *argv[])
{
	int N,pid;
	N = atoi(argv[1]);
	char child_ID[5];
	int mapper_fd[N][2];

	if(argc == 3){
		createpipe(mapper_fd,N);	//create mapper pipes
		getlines(mapper_fd,N); 		//taking inputs on parent process after creating pipes
		for(int i =0;i<N;i++){
			pid=fork();
			if(pid>0){ 				//Parent Process
				close(mapper_fd[i][0]);		
			}
			else if(pid==0){		//Child Process
				dup2(mapper_fd[i][0],0);
				closeall(mapper_fd,N);
				sprintf(child_ID,"%d",i);
				execl(argv[2],argv[2],child_ID,(char *)0);
			}
		}
		closeall(mapper_fd,N);
		for(int i=0;i<N;i++){
			wait(NULL);
		}

	}
	else{

	}

}

