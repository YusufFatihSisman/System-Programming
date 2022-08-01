#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<string.h>
#include <ctype.h>

#include "helper.h"

int becomeDaemon(int flags){
	int maxfd, fd;
	switch(fork()){
		case -1: return -1;
		case 0: break;
		default: _exit(EXIT_SUCCESS);
	}

	if(setsid() == -1)
		return -1;

	switch(fork()){
		case -1: return -1;
		case 0: break;
		default: _exit(EXIT_SUCCESS);
	}

	if(!(flags & BD_NO_UNMASK0))
		//unmask(0);
	if(!(flags & BD_NO_CHDIR))
		chdir("/");
	if(!(flags & BD_NO_CLOSE_FILES)){
		maxfd = sysconf(_SC_OPEN_MAX);
		if(maxfd == -1)
			maxfd = BD_MAX_CLOSE;
		for(fd = 0; fd < maxfd; fd++){
			close(fd);
		}
	}
	if(!(flags & BD_NO_REOPEN_STD_FDS)){
		close(STDIN_FILENO);
		fd = open("/dev/null", O_RDWR);
		if(fd != STDIN_FILENO)
			return -1;
		if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
			return -1;
		if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
			return -1;
	}
	return 0;
}

void freeTable(){
	int i, j;
	for(i = 0; i < table.sizeY; i++){
		for(j = 0; j < table.sizeX; j++){
			free(table.memory[i][j]);
		}
		free(table.memory[i]);
	}
	free(table.memory);
}

void update(char **args, int argSize, char* whereArg, int socket_fd){
	char result[MAXLINELENGTH];
	int index = 0;
	int t;
	int length;

	char** befores = (char**)malloc(argSize * sizeof(char*));
	char** afters = (char**)malloc(argSize * sizeof(char*)); 
	char* beforeWhere = (char*)malloc(strlen(whereArg));
	char* afterWhere = (char*)malloc(strlen(whereArg));
	
	int i;
	for(i = 0; i < argSize; i++){
		befores[i] = (char*)malloc(strlen(args[i]));
		afters[i] = (char*)malloc(strlen(args[i]));
		beforeEquation(args[i], befores[i]);
		afterEquation(args[i], afters[i]);
	}
	beforeEquation(whereArg, beforeWhere);
	afterEquation(whereArg, afterWhere);
	
	int whereColumn = -1;
	
	for(i = 0; i < table.sizeX; i++){
		if(strcmp(table.memory[0][i], beforeWhere) == 0){
			whereColumn = i;
			i = table.sizeX;
		}
	}
	if(whereColumn == -1){
		printf("NOT FOUND\n");
		return;
	}
	int j;
	int k = 0;

	for(k = 0; k < argSize; k++){
		length = strlen(befores[k]);
		for(t = 0; t < length; t++){
			result[index] = befores[k][t];
			index++;
		}
		result[index++] = '\t';
	}
	result[index] = '\n';
	write(socket_fd, result, index+1);
	index = 0;

	for(i = 0; i < table.sizeY; i++){
		if(strcmp(table.memory[i][whereColumn], afterWhere) == 0){
			for(j = 0; j < table.sizeX; j++){
				for(k = 0; k < argSize; k++){
					if(strcmp(table.memory[0][j], befores[k]) == 0){
						free(table.memory[i][j]);
						table.memory[i][j] = (char*)malloc(strlen(afters[k]) + 1);
						strcpy(table.memory[i][j], afters[k]);
						length = strlen(afters[k]);
						for(t = 0; t < length; t++){
							result[index] = table.memory[i][j][t];
							index++;
						}
						result[index++] = '\t';		
					}
				}				
			}
			result[index] = '\n';
			write(socket_fd, result, index+1);
			index = 0;	
		}
	}	

	for(i = 0; i < argSize; i++){
		free(befores[i]);
		free(afters[i]);
	}
	free(befores);
	free(afters);
	free(beforeWhere);
	free(afterWhere);

	result[0] = '\n';
	write(socket_fd, result, 1);
}


void selectPrint(enum OP type, char **args, int argSize, int socket_fd){
	int arr[table.sizeX];
	int i = selectFunc(type, args, argSize, arr);
	int k, j;
	char result[MAXLINELENGTH];
	int index = 0;
	int t;
	int length;
	if(i == -1){
		for(i = 0; i < table.sizeY; i++){
			for(j = 0; j < table.sizeX; j++){
				length = strlen(table.memory[i][j]);
				for(t = 0; t < length; t++){
					result[index] = table.memory[i][j][t];
					index++;
				}
				result[index++] = '\t';		
			}
			result[index] = '\n';
			write(socket_fd, result, index+1);
			index = 0;	
		}
		result[0] = '\n';
		write(socket_fd, result, 1);
	}else{
		for(i = 0; i < table.sizeY; i++){
			for(j = 0; j < table.sizeX; j++){
				for(k = 0; k < table.sizeX; k++){
					if(arr[k] == 1 && k == j){
						length = strlen(table.memory[i][j]);
						for(t = 0; t < length; t++){
							result[index] = table.memory[i][j][t];
							index++;
						}
						result[index++] = '\t';
					}	
				}				
			}
			result[index] = '\n';
			write(socket_fd, result, index+1);
			index = 0;
		}
		result[0] = '\n';
		write(socket_fd, result, 1);
	}
}

int selectFunc(enum OP type, char **args, int argSize, int *arr){
	int i = 0;
	int j = 0;
	if(strcmp(args[0], "*") == 0){
		return -1;
	}	
	for(i = 0; i < table.sizeX; i++){
		for(j = 0; j < argSize; j++){
			if(strcmp(table.memory[0][i], args[j]) == 0){
				arr[i] = 1;
				j = argSize;
			}else{
				arr[i] = 0;
			}
			
		}	
	}
	return 0;
}

void afterEquation(char *command, char *temp){
	int size = strlen(command);
	int i = 0;
	int j = 0;
	for(i = 0; i < size; i++){
		if(command[i] == '='){
			j = i + 1;
			i = size;
		}
	}
	i = 0;
	while(j < size){
		if(command[j] != '\''){
			temp[i] = command[j];
			i++;
		}
		j++;
	}
	temp[i] = '\0';
}

void beforeEquation(char *command, char *temp){
	int size = strlen(command);
	int i = 0;
	for(i = 0; i < size; i++){
		if(command[i] != '='){
			temp[i] = command[i];
		}else{
			temp[i] = '\0';
		}
	}
}

int parser(char* command, char ***args, char **whereArg, int* argSize){
	int arg = 0;
	char buff[MAXCOMMANDLENGTH];
	int i = 0;
	int size = strlen(command);
	int buffIndex = 0;
	int comma = 0;
	int type = -1;
	int idLength = 0;
	
	for(i = 0; i < size; i++){
		if(command[i] == ' '){
			idLength = i;
			i = size;
		}
	}
	type = commandType(command, idLength);
	if(type == -1){
		printf("INVALID COMMAND\n");
		return -1;
	}
	if(type == SELECT){
		i = idLength + 6 + 2;
	}else if(type == DISTINCT){
		i = idLength + 15 + 2;
	}else if(type == UPDATE){
		i = idLength + 16 + 2;
	}
	
	int first = 1;
	int where = 0;
	while(i < size){
		if(command[i] == ' ' || command[i] == ';'){
			buff[buffIndex] = '\0';	
			if(first == 1 || comma > 0){
				arg++;
				if(arg == 1){
					(*args) = (char **)malloc(arg * sizeof(char *));
				}else{
					(*args) = (char **)realloc((*args), arg * sizeof(char *));
				}
				(*args)[arg - 1] = (char *)malloc(strlen(buff) + 1);
				strcpy((*args)[arg-1], buff);
				if(first == 1){
					first = 0;
				}else{
					comma -= 1;
				}
			}else if(where == 1){
				(*whereArg) = (char *)malloc(strlen(buff) + 1);
				strcpy((*whereArg), buff);
			}else{
				if(isWhere(buff) == 0){
					where = 1;
				}else{
					printf("INVALID COMMAND\n");
					return -1;
				}
			}
			buffIndex = 0;
		}else if(command[i] == ','){
			if(command[i + 1] != ' '){
				buff[buffIndex] = '\0';	
				arg++;
				if(arg == 1){
					(*args) = (char **)malloc(arg * sizeof(char *));
				}else{
					(*args) = (char **)realloc((*args), arg * sizeof(char *));
				}
				(*args)[arg - 1] = (char *)malloc(strlen(buff) + 1);
				strcpy((*args)[arg-1], buff);
				if(first == 1){
					first = 0;
					comma += 1;
				}else{
					comma -= 0;
				}
				
				buffIndex = 0;
			}else{
				comma += 1;	
			}
			
		}else{
			buff[buffIndex] = command[i];
			buffIndex++;
			if(command[i] == 'F'){
				if(type == 0 || type == 1){
					if(isFromTable(command, i) == 0){
						i += 11;
					}else{
						printf("INVALID COMMAND\n");
						return -1;
					}
				}
			}
		}
		i++;
	}
	*argSize = arg;
	return type;
}

int isWhere(char* command){
	int i = 0;
	int size = strlen(command);
	if(size == strlen(STR_WHERE)){
		for(i = 0; i < size; i++){
			if(command[i] != STR_WHERE[i]){
				return -1;
			}
		}
	}else{
		return -1;
	}
	return 0;
}

int isFromTable(char* command, int index){
	char from[11];
	memcpy(from, &command[index], 10);
	from[10] = '\0';
	int i = 0;
	int size = strlen(from);
	if(size == strlen(STR_FROM_TABLE)){
		for(i = 0; i < size; i++){
			if(from[i] != STR_FROM_TABLE[i]){
				return -1;
			}
		}
	}else{
		return -1;
	}
	return 0;
}

int commandType(char *command, int idLength){
	char sel[7];
	char dist[16];
	char update[17];
	
	memcpy(dist, &command[idLength + 1], 15);
	dist[15] = '\0';
	if(isDistinct(dist) == 0){
		return DISTINCT;
	}
	memcpy(sel, &command[idLength + 1], 6);
	sel[6] = '\0';
	if(isSelect(sel) == 0){
		return SELECT;
	}
	memcpy(update, &command[idLength + 1], 16);
	update[16] = '\0';
	if(isUpdate(update) == 0){
		return UPDATE;
	}
	return -1;
}

int isSelect(char *command){
	int i = 0;
	int size = strlen(command);
	if(size == strlen(STR_SELECT)){
		for(i = 0; i < size; i++){
			if(command[i] != STR_SELECT[i]){
				return -1;
			}
		}
	}else{
		return -1;
	}
	return 0;
}

int isDistinct(char *command){
	int i = 0;
	int size = strlen(command);
	if(size == strlen(STR_SELECT_DISTINCT)){
		for(i = 0; i < size; i++){
			if(command[i] != STR_SELECT_DISTINCT[i]){
				return -1;
			}
		}
	}else{
		return -1;
	}
	return 0;
}

int isUpdate(char *command){
	int i = 0;
	int size = strlen(command);
	if(size == strlen(STR_UPDATE)){
		for(i = 0; i < size; i++){
			if(command[i] != STR_UPDATE[i]){
				return -1;
			}
		}
	}else{
		return -1;
	}
	return 0;
}


void csvRead(char* path){
	
	table.sizeX = 0;
	table.sizeY = 0;
	table.capacityY = 2;
	table.memory = (char***)malloc((table.capacityY + 1)*sizeof(char**));
	
	int fd = open(path, O_RDONLY, 0644);
	if(fd == -1){
		perror("open file: ");
		exit(EXIT_FAILURE);
	}
	char c[1];
	char buffer[MAXLINELENGTH];
	int size;
	int i = 0;
	int x = 0;
	int y = 0;
	int inquote = 0;
	
	while(1){
		size = read(fd, c, 1);
		if(size == -1){
			perror("read file: ");
			exit(EXIT_FAILURE);	
		}else if(size == 0){
			break;
		}
		if(c[0] == '\n'){
			table.sizeX = x + 1;
			break;
		}
		if(c[0] == '"'){
			if(inquote == 0)
				inquote = 1;
			else if(inquote == 1)
				inquote = 0;
		}
		if(c[0] == ',' && inquote == 0){
			x++;
		}	
	}
	table.memory[0] = (char**)malloc((table.sizeX + 1) * sizeof(char*));	
	lseek(fd, 0, SEEK_SET);
	x = 0;
	inquote = 0;
	y = 0;
	i = 0;
	
	while(1){
		size = read(fd, c, 1);
		if(size == -1){
			perror("read file: ");
			exit(EXIT_FAILURE);	
		}else if(size == 0){
			break;
		}
		
		if(inquote == 1){
			buffer[i] = c[0];
			i++;
		}else if(c[0] != '\n' && c[0] != '\r' && c[0] != ','){
			buffer[i] = c[0];
			i++;
		}
		
		
		if(c[0] == '\n'){
			if(i != 0){
				if(y >= table.capacityY - 1){
					table.capacityY *= 2;
					table.memory = (char***)realloc(table.memory, table.capacityY * sizeof(char**));	
				}
				table.memory[y + 1] = (char**)malloc((table.sizeX + 1) * sizeof(char*));
			
				buffer[i] = '\0';
				table.memory[y][x] = (char*)malloc(strlen(buffer) + 1);
				strcpy(table.memory[y][x], buffer);
				
				y++;
				table.sizeY = y;
				x = 0;
				i = 0;
			}		
		}
		if(c[0] == '"'){
			if(inquote == 0)
				inquote = 1;
			else if(inquote == 1)
				inquote = 0;
		}
		
		if(c[0] == ',' && inquote == 0){
			buffer[i] = '\0';
			table.memory[y][x] = (char*)malloc(strlen(buffer) + 1);
			strcpy(table.memory[y][x], buffer);
			x++;
			i = 0;
		}
	}
	free(table.memory[table.sizeY]);	
	close(fd);
}

void getArgs(int argc, char **argv, int *port, char **logFilePath, int *poolSize, char **datasetPath){
	
	int c = 0;

	while((c = getopt(argc, argv, "p:o:l:d:")) != -1){
		switch(c){
			case 'p':
				*port = atoi(optarg);
				break;
			case 'o':
				*logFilePath = optarg;
				break;
			case 'l':
				*poolSize = atoi(optarg);
				break;
			case 'd':
				*datasetPath = optarg;
				break;
			case '?':
				if(optopt == 'p' || optopt == 'o' 
					|| optopt == 'l' || optopt == 'd'){
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
	if(*poolSize < 2){
		printf("Value of pool size must be greater than 1\n");
		printf("Proper way of execute this program:\n");
		printf("./server -p PORT -o pathToLogFile -l poolSize -d datasetPath\n");
		exit(EXIT_FAILURE);
	}
	if(*port <= 1000){
		printf("Value of port number must be greater than 1000\n");
		printf("Proper way of execute this program:\n");
		printf("./server -p PORT -o pathToLogFile -l poolSize -d datasetPath\n");
		exit(EXIT_FAILURE);
	}
}