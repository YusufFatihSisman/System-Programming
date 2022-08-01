#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX 5
#define MAX_ID 5
#define BUFSIZE 200

void getArgs(int argc, char **argv, int *id, char **ipv4, int *port, char **queryPath);

int query = 0;

void func(int socket_fd, char *queryPath, int id){
	FILE* fp;
	char* command = NULL;
	size_t len = 0;
	int size; 
	char readedId[MAX_ID + 1];
	int i;
	char buffer[BUFSIZE];
	size_t bytes_read;
	int index = 0;
	int whileCond = 0;

	fp = fopen(queryPath, "r");
	if(fp == NULL){
		fprintf(stderr, "Error occur while trying to open file %s\n", queryPath);
		exit(EXIT_FAILURE);
	}
    
	while((size = getline(&command, &len, fp)) != -1){
		whileCond = 0;
		for(i = 0; i < MAX_ID + 1; i++){
			if(command[i] == ' '){
				readedId[i] = '\0';
				i = MAX_ID + 1;
			}else{
				readedId[i] = command[i];
			}
		}
		if(id == atoi(readedId)){
			printf("Client-%d connected and sending query '%s'\n", id, command);
			write(socket_fd, command, strlen(command));
			query++;
			while(whileCond == 0){
				char c[1];
				while(1){
					bytes_read = read(socket_fd, c, 1);
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
					index = 0;
					whileCond = 1;
				}else{
					buffer[index-1] = '\0';
					printf("%s\n", buffer);
					index = 0;
				}
			}
		}
	}
	write(socket_fd, "\n", 1);
	fclose(fp);
	printf("A total of %d queries were executed, Client is terminating.\n", query);
	return;
}

int main(int argc, char **argv){
	
	int id;
	char *ipv4;
	int port;
	char *queryPath;

	int socket_fd;
	struct sockaddr_in server_address;

	getArgs(argc, argv, &id, &ipv4, &port, &queryPath);

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd == -1){
		perror("socket create failed: ");
		exit(EXIT_FAILURE);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(ipv4);
	server_address.sin_port = htons(port);

	if(connect(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1){
		perror("Connection failed: ");
		exit(EXIT_FAILURE);
	}
	printf("Client-%d connecting to %s\n", id, ipv4);
	func(socket_fd, queryPath, id);

	return 1;
}


void getArgs(int argc, char **argv, int *id, char **ipv4, int *port, char **queryPath){
	int c = 0;

	while((c = getopt(argc, argv, "i:a:p:o:")) != -1){
		switch(c){
			case 'i':
				*id = atoi(optarg);
				break;
			case 'a':
				*ipv4 = optarg;
				break;
			case 'p':
				*port = atoi(optarg);
				break;
			case 'o':
				*queryPath = optarg;
				break;
			case '?':
				if(optopt == 'i' || optopt == 'a' 
					|| optopt == 'p' || optopt == 'o'){
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				}else if(isprint(optopt)){
					fprintf(stderr, "Unkown option -%c.\n", optopt);
				}else{
					fprintf(stderr, "Unkown option character \\x%x.\n", optopt);
				}
				exit(EXIT_FAILURE);
			default:
				break;
		}
	}
	if(*id < 1){
		printf("Value of id must be greater than 0\n");
		printf("Proper way of execute this program:\n");
		printf("./client -i id -a 127.0.0.1 -p PORT -o pathToQueryFile\n");
		exit(EXIT_FAILURE);
	}

	if(*port <= 1000){
		printf("Value of port number must be greater than 1000\n");
		printf("Proper way of execute this program:\n");
		printf("./client -i id -a 127.0.0.1 -p PORT -o pathToQueryFile\n");
		exit(EXIT_FAILURE);
	}
}
