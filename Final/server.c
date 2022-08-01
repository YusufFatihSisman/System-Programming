#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include "queue.h"
#include "helper.h"

#define SERVER_BACKLOG 20
#define BUFSIZE 100
#define PATH_MAX 100

pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t new_socket = PTHREAD_COND_INITIALIZER;
pthread_cond_t okToRead = PTHREAD_COND_INITIALIZER;
pthread_cond_t okToWrite = PTHREAD_COND_INITIALIZER;
pthread_cond_t end_cond = PTHREAD_COND_INITIALIZER;

sig_atomic_t ctrl_c = 0;
struct sigaction sigIntHandler;

void ctrlc_handler(){
	ctrl_c = 1;
}

Queue *clients;
struct Table table;

int AR = 0;
int AW = 0;
int WR = 0;
int WW = 0;

void* thread(void *arg);
void handler(int client_socket, int threadID); 
void writer(char** args, char* whereArg, int argSize, int socket_fd);
void reader(char** args, char* whereArg, int argSize, int socket_fd);

int main(int argc, char **argv){
	//becomeDaemon(0);
	int port;
	char *logFilePath;
	int poolSize;
	char *datasetPath;

	int server_socket;
	int client_socket;
	int addrSize;

	struct sockaddr_in server_address;
	struct sockaddr_in client_address;

	getArgs(argc, argv, &port, &logFilePath, &poolSize, &datasetPath);

	sigIntHandler.sa_handler = &ctrlc_handler;
   	sigIntHandler.sa_flags = 0;
   	if((sigemptyset(&sigIntHandler.sa_mask) == -1) || (sigaction(SIGINT, &sigIntHandler, NULL) == -1)){
   		perror("Failed to install SIGINT signal handler");
   	}
   	if(ctrl_c == 1){
   		return 0;
   	}
	csvRead(datasetPath);
	if(ctrl_c == 1){
		freeTable();
	}

	clients = createQueue();
	pthread_t threads[poolSize];
	int i;
	int ids[poolSize];
	for(i = 0; i < poolSize; i++){
		ids[i] = i;
		if(pthread_create(&threads[i], NULL, &thread, (void*)&ids[i]) != 0){
			fprintf(stderr, "thread could not be created.\n");
			exit(EXIT_FAILURE);
		}
	}

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket == -1){
		perror("socket create failed: ");
		exit(EXIT_FAILURE);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port);

	if((bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address))) == -1){
		perror("socket bind failed: ");
		exit(EXIT_FAILURE);
	}

	if((listen(server_socket, SERVER_BACKLOG)) == -1){
		perror("socket listen failed: ");
		exit(EXIT_FAILURE);
	}

	while(1){
		printf("Waiting connections...\n");
		if(ctrl_c == 1){
			break;
		}

		addrSize = sizeof(struct sockaddr_in);
		client_socket = accept(server_socket, (struct  sockaddr*)&client_address, (socklen_t*)&addrSize);
		if(client_socket == -1){
			perror("accept failed: ");
			exit(EXIT_FAILURE);
		}
		printf("Connected\n");

		pthread_mutex_lock(&socket_mutex);
		enQueue(clients, client_socket);
		pthread_cond_signal(&new_socket);
		pthread_mutex_unlock(&socket_mutex);

		if(ctrl_c == 1){
			break;
		}
	}
	pthread_mutex_lock(&socket_mutex);
	pthread_cond_wait(&end_cond, &socket_mutex);

	freeTable();
	return 0;

}

void* thread(void *arg){
	int threadID = *((int*)arg);
	printf("Thread #%d: waiting for connection\n", threadID);
	while(1){
		int pclient;
		pthread_mutex_lock(&socket_mutex);
		if((pclient = deQueue(clients)) == -1){
			pthread_cond_wait(&new_socket, &socket_mutex);
			pclient = deQueue(clients);
		}
		pthread_mutex_unlock(&socket_mutex);
		if(pclient != -1){
			handler(pclient, threadID);
		}
	}
}

void reader(char** args, char* whereArg, int argSize, int socket_fd){
	pthread_mutex_lock(&condMutex);
	while((AW + WW) > 0){
		WR++;
		pthread_cond_wait(&okToRead, &condMutex);
		WR--;
	}
	AR++;
	pthread_mutex_unlock(&condMutex);
	selectPrint(SELECT, args, argSize, socket_fd);
	pthread_mutex_lock(&condMutex);
	AR--;
	if(AR == 0 && WW > 0){
		pthread_cond_signal(&okToWrite);
	}
	pthread_mutex_unlock(&condMutex);
}

void writer(char** args, char* whereArg, int argSize, int socket_fd){
	pthread_mutex_lock(&condMutex);
	while((AW + AR) > 0){
		WW++;
		pthread_cond_wait(&okToWrite, &condMutex);
		WW--;
	}
	AW++;
	pthread_mutex_unlock(&condMutex);
	update(args, argSize, whereArg, socket_fd);
	pthread_mutex_lock(&condMutex);
	AW--;
	if(WW > 0){
		pthread_cond_signal(&okToWrite);
	}else if(WR > 0){
		pthread_cond_broadcast(&okToRead);
	}
	pthread_mutex_unlock(&condMutex);
}

void handler(int client_socket, int threadID){
	char buffer[BUFSIZE];
	size_t bytes_read;
	int index = 0;
	int type;
	int i;
	while(1){
		char c[1];
		while(1){
			bytes_read = read(client_socket, c, 1);
			if(bytes_read == -1){
				perror("read: ");
				exit(EXIT_FAILURE);
			}
			buffer[index++] = c[0];
			if(c[0] == '\n'){
				break;
			} 
		}
		if(buffer[0] == '\n'){
			printf("client closed\n");
			close(client_socket);
			return;
		}
	
		buffer[index-1] = 0;
		printf("Thread #%d: received query \'%s\'\n", threadID, buffer);
		index = 0;

		char** args;
		char* whereArg;
		int argSize;

		type = parser(buffer, &args, &whereArg, &argSize);
		if(type == -1){
			printf("ERROR: INVALID COMMAND\n");
			//return;
		}else if(type == UPDATE){
			writer(args, whereArg, argSize, client_socket);
			free(whereArg);
		}else{
			reader(args, whereArg, argSize, client_socket);
		}
		
		for(i = 0; i < argSize; i++){
			free(args[i]);
		}
		free(args);

		pthread_mutex_lock(&condMutex);
		//ctrl + c
		if((AR + WR + AW + WW) == 0){
			if(ctrl_c == 1){
				pthread_cond_signal(&end_cond);
			}
		}
		//ctrl + c
		pthread_mutex_unlock(&condMutex);
		usleep(500000);
	}
	
	return;
}

