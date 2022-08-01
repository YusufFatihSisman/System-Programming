#include<stdio.h>
#include<ctype.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<dirent.h>

int isNum(char *checked){
	int length = strlen(checked);
	int i;

	for(i=0; i<length; i++){
		if(isdigit(checked[i]) == 0){
			return -1;
		}
	}
	return 0;
}

int isValidType(char *checked){
	if(strlen(checked) > 1){
		return -1;
	}
	if(checked[0] != 'd' && checked[0] != 's' && checked[0] != 'b'
		&& checked[0] != 'c' && checked[0] != 'f' && checked[0] != 'p'
		&& checked[0] != 'l'){
		return -1;
	}
	return 0;
}

int equal(char a, char b){
	if(a == b || a == toupper(b) || toupper(a) == (b)){
		return 0;
	}else{
		return -1;
	}
}

int nameCheck(char *wanted, char *checked){
	int i;
	int j = 0;
	int lWant = strlen(wanted);
	int lCheck = strlen(checked);
	
	if(lWant > lCheck+1 || wanted[0] == '+'){
		return -1;
	}
	
	for(i = 0; i < lCheck; i++){
		if(wanted[j] == '+'){
			if(equal(checked[i-1], checked[i]) == -1){
				j++;
				i--;
			}
		}else if(equal(wanted[j], checked[i]) == -1){
			return -1;
		}else{
			j++;
		}
		if(j > lWant){
			return -1;
		}
	}
	
	return 0;
}

char getType(struct stat st){
	switch(st.st_mode & S_IFMT){
		case S_IFBLK:
			return 'b';
		case S_IFCHR:
			return 'c';
		case S_IFDIR:
			return 'd';
		case S_IFIFO:
			return 'p';
		case S_IFLNK:
			return 'l';
		case S_IFREG:
			return 'r';
		case S_IFSOCK:
			return 's';
		default:
			perror("stat");
			exit(EXIT_FAILURE);
	}
}

void getPermissions(char temp[9], struct stat st){
	(st.st_mode & S_IRUSR) ? (temp[0] = 'r') : (temp[0] = '-'); 
    (st.st_mode & S_IWUSR) ? (temp[1] = 'w') : (temp[1] = '-'); 
    (st.st_mode & S_IXUSR) ? (temp[2] = 'x') : (temp[2] = '-'); 
    (st.st_mode & S_IRGRP) ? (temp[3] = 'r') : (temp[3] = '-');  
    (st.st_mode & S_IWGRP) ? (temp[4] = 'w') : (temp[4] = '-');  
    (st.st_mode & S_IXGRP) ? (temp[5] = 'x') : (temp[5] = '-');  
    (st.st_mode & S_IROTH) ? (temp[6] = 'r') : (temp[6] = '-');  
    (st.st_mode & S_IWOTH) ? (temp[7] = 'w') : (temp[7] = '-');  
    (st.st_mode & S_IXOTH) ? (temp[8] = 'x') : (temp[8] = '-');  
}

void traverse(char *path, char *fileName, int fileSize, char fileType, char *permission, unsigned numLinks, int isNumLinks, int *found, char ***foundeds, int* maxSize){
	
	struct dirent *current;
	DIR *dir = opendir(path);
	int isMatch = 1;
	
	if(dir == NULL){
		fprintf(stderr, "Directory could not opened!\n");
		exit(EXIT_FAILURE);
	}
	while((current = readdir(dir)) != NULL){
		char *newPath = malloc(2 + strlen(path) + strlen(current->d_name));
		if(newPath == NULL){
			fprintf(stderr, "Malloc could not succeeded!\n");
		}
		strcpy(newPath, path);
		strcat(newPath, "/");
		strcat(newPath, current->d_name);
		if(current->d_type == DT_DIR){
			if((strcmp(current->d_name, ".") != 0) && (strcmp(current->d_name, "..") != 0)){	
				traverse(newPath, fileName, fileSize, fileType, permission, numLinks, isNumLinks, found, foundeds, maxSize);
			}
		}
		isMatch = 1;
		if((strcmp(current->d_name, ".") != 0) && (strcmp(current->d_name, "..") != 0)){
			if(fileName != NULL){
				if(nameCheck(fileName, current->d_name) == -1){
					isMatch = 0;
				}
			}
			if(isMatch != 0 && fileSize != -1){
				struct stat st;
				if(stat(newPath, &st) == -1){
					fprintf(stderr, "When tyring to access the stat of file with path %s, an error occured\n", newPath);
				}
				if(st.st_size != fileSize){
					isMatch = 0;
				}
			}
			if(isMatch != 0 && isNumLinks == 1){
				struct stat st;
				if(stat(newPath, &st) == -1){
					fprintf(stderr, "When tyring to access the stat of file with path %s, an error occured\n", newPath);
				}
				if(st.st_nlink != numLinks){
					isMatch = 0;
				}
			}
			if(isMatch != 0 && fileType != '\0'){
				struct stat st;
				if(stat(newPath, &st) == -1){
					fprintf(stderr, "When tyring to access the stat of file with path %s, an error occured\n", newPath);
				}
				if(getType(st) != fileType){
					isMatch = 0;
				}
			}
			if(isMatch != 0 && permission != NULL){
				struct stat st;
				char temp[9];
				if(stat(newPath, &st) == -1){
					fprintf(stderr, "When tyring to access the stat of file with path %s, an error occured\n", newPath);
				}
				getPermissions(temp, st);
				if(strcmp(temp, permission) != 0){
					isMatch = 0;
				}
			}
			if(isMatch == 1){
				if(*maxSize > *found + 1){
					(*foundeds)[*found] = malloc(strlen(newPath) + 1);
					if((*foundeds)[*found] == NULL){
						fprintf(stderr, "Malloc could not succeeded!\n");
					}
				}else{
					*maxSize = *maxSize * 2; 
					*foundeds = realloc(*foundeds, (*maxSize + 1) * sizeof(char*));
					if(*foundeds == NULL){
						fprintf(stderr, "Malloc could not succeeded!\n");
					}
					(*foundeds)[*found] = malloc(strlen(newPath) + 1);
					if((*foundeds)[*found] == NULL){
						fprintf(stderr, "Malloc could not succeeded!\n");
					}
				}
				strcpy((*foundeds)[*found], newPath);
				(*found)++;
			}
		}
		free(newPath);
	}
	closedir(dir);
}



int main(int argc, char **argv){

	opterr = 0;

	int isPath = 0;
	int isSearch = 0;
	int isNumLinks = 0;
	char *path = NULL;
	char *fileName = NULL;
	char *permission = NULL;
	unsigned numLinks = -1;
	int fileSize = -1;
	char fileType = '\0';
	int maxSize = 8;
	int found = 0;
	char **foundeds = malloc(maxSize * sizeof(char*));
	int c;

	while((c = getopt(argc, argv, "w:f:b:t:p:l:")) != -1){

		if(c == 'w'){
			if(optarg[0] == '-'){
				fprintf(stderr, "You have to enter path name and path names can not begin with '-'\n");
				return -1;
			}
			isPath = 1;
			path = optarg;
		}else if(c == 'f'){
			if(optarg[0] == '-'){
				fprintf(stderr, "%s is not a valid file name. You have to enter a valid file name and file names can not begin with '-'\n", optarg);
				return -1;
			}
			isSearch = 1;
			fileName = optarg;
		}else if(c == 'b'){
			if(isNum(optarg) == -1){
				fprintf(stderr, "%s is not a valid file size. You have to enter valid file size. (eg. 100)\n", optarg);
				return -1;
			}
			isSearch = 1;
			fileSize = atoi(optarg);
		}else if(c == 't'){
			if(isValidType(optarg) == -1){
				fprintf(stderr, "%s is not a valid file type. You can try d, s, b, c, f, p or l\n", optarg);
				return -1;
			}
			isSearch = 1;
			fileType = optarg[0];
		}else if(c == 'p'){
			if(strlen(optarg) != 9){
				fprintf(stderr, "You have to enter permission with 9 character. %s is not 9 character.\n", optarg);
				return -1;
			}
			isSearch = 1;
			permission = optarg;
		}else if(c == 'l'){
			if(isNum(optarg) == -1){
				fprintf(stderr, "%s is not a valid number of links. You have to enter a number", optarg);
				return -1;
			}
			isNumLinks = 1;
			isSearch = 1;
			numLinks = atoi(optarg);
		}else if(c == '?'){
			if (optopt == 'w' || optopt == 'f' || optopt == 'b' || optopt == 't' || optopt == 'p' || optopt == 'l'){
				fprintf(stderr, "%c option requires an argument.\n", optopt);
			}else if(isprint (optopt)){
				fprintf(stderr, "Unknown option %c\n", optopt);
			}
			return -1;
		}
	}

	if(isPath == 0){
		fprintf(stderr, "Path is required\n");
		return -1;
	}

	if(isSearch == 0){
		fprintf(stderr, "At least one search criteria required\n");
		return -1;
	}
	
	traverse(path, fileName, fileSize, fileType, permission, numLinks, isNumLinks, &found, &foundeds, &maxSize);

	if(found == 0){
		printf("No file found\n");
	}else{
		int i;
		printf("Foundeds:\n");
		for(i=0; i<found; i++){
			printf("%s\n", foundeds[i]);
		}
		for(i=0; i<found; i++){
			free(foundeds[i]);
		}
	}
	free(foundeds);
	
	return 0;
}