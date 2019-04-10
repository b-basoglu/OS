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
void closeallmapreducer_fd(int fd1[][2],int fd2[][2],int fd3[][2],int N){
	closeall(fd1,N);
	closeall(fd2,N);
	closeall(fd3,N);
}
void getlines(int fd[][2],int N){
	char input[1024];
	for(int i=0;fgets(input,1024,stdin);i++){
		write(fd[(i%N)][1],input,strlen(input));
	}
}
void waitforclose(int N){
	for(int i=0;i<N;i++){
		wait(NULL);
	}
}
void mapper(int N,char* arg){
	int mapper_fd[N][2];
	char child_ID[10];
	int pid;
	createpipe(mapper_fd,N);			//create mapper pipes	
	for(int i =0;i<N;i++){
		pid=fork();
		if(pid<0){
			fprintf(stderr, "fork Failed" );
			exit(1);
		}
		else if(pid==0){				//Child Process
			dup2(mapper_fd[i][0],0); 	//parent ---- child
			closeall(mapper_fd,N);		//close all pipes
			sprintf(child_ID,"%d",i);
			execl(arg,arg,child_ID,(char *)0);
		}
	}
	getlines(mapper_fd,N); 				//takes inputs on parent process after creating pipes
	closeall(mapper_fd,N); 				//close all mapper pipes

}
void mapreducer(int N,char* arg1,char*arg2){
	int mapper_fd[N][2];
	int mapperreducer_fd[N][2];
	int reducerreducer_fd[N][2];
	char child_ID[10];
	int pid,pid2;
	createpipe(mapper_fd,N);
	createpipe(mapperreducer_fd,N);
	createpipe(reducerreducer_fd,N);
	for(int i =0;i<N;i++){
		pid=fork();
		if(pid<0){
			fprintf(stderr, "fork Failed" );
			exit(EXIT_FAILURE);
		}
		else if(pid==0){									//child				
			dup2(mapper_fd[i][0],0);
			dup2(mapperreducer_fd[i][1],1);
			closeallmapreducer_fd(mapper_fd,mapperreducer_fd,reducerreducer_fd,N);
			
			sprintf(child_ID,"%d",i);
			execl(arg1,arg1,child_ID,(char *)0);
		}
	}
	for(int i =0;i<N;i++){
		pid2=fork();
		if(pid2<0){
			fprintf(stderr, "fork Failed" );
			exit(EXIT_FAILURE);
		}
		else if(pid2==0){									//child
			dup2(mapperreducer_fd[i][0],0);
			if(N>1){
				if (i == 0 )
				{
					dup2(reducerreducer_fd[i][1],1);
					
					
				}else if (i== N-1)
				{

					dup2(reducerreducer_fd[i-1][0],2);
					
					
				}else{
					dup2(reducerreducer_fd[i][1],1);
					dup2(reducerreducer_fd[i-1][0],2);
				}

			}
			closeallmapreducer_fd(mapper_fd,mapperreducer_fd,reducerreducer_fd,N);
			sprintf(child_ID,"%d",i);
			execl(arg2,arg2,child_ID,(char *)0);			
		}
	}
	getlines(mapper_fd,N); 				//takes inputs on parent process after creating pipes
	closeallmapreducer_fd(mapper_fd,mapperreducer_fd,reducerreducer_fd,N);	

}

int main(int argc, char *argv[])
{
	int N;				//number of mappers
	N = atoi(argv[1]);
	
	if(argc == 3){
		mapper(N,argv[2]);
		waitforclose(N);
		exit(EXIT_SUCCESS);

	}
    else if(argc == 4){
		mapreducer(N,argv[2],argv[3]);
		waitforclose(2*N);
		exit(EXIT_SUCCESS);
    }
    else{
    	fprintf(stderr, "Argument count is wrong!" );
    	exit(EXIT_FAILURE);
    }

}
