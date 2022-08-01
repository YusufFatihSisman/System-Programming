#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include "helper.h"

int fillHireds(int fd){
	int size;
	char c[1];
	int amount = 0;
	int total = 0;
	char *pt = "";
	char *check =  " \n";
	int i = 0;
	int j = 0;

	do{
		size = read(fd, c, 1);
		if(size == -1){
			perror("read file");
			exit(EXIT_FAILURE);
		}
		if(c[0] == '\n' && size != 0){
			amount++;
		}
		total++;
	}while(size != 0);

	hireds = malloc(sizeof(Hired) * amount);
	if(hireds == NULL){
		fprintf(stderr, "Malloc is failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}

	if(lseek(fd, 0, SEEK_SET) == -1){
		perror("lseek");
		exit(EXIT_FAILURE);
	}

	char buffer[total];

	size = read(fd, buffer, total - 1);
	if(size == -1){
		perror("read file");
		exit(EXIT_FAILURE);
	}
	buffer[total - 1] = '\0';

	pt = strtok(buffer, check);
	while(pt != NULL && j < amount){
		if(i == 0){
			hireds[j].name = malloc(strlen(pt) + 1);
			if(hireds[j].name == NULL){
				fprintf(stderr, "Malloc is failed to allocate memory.\n");
				exit(EXIT_FAILURE);
			}
			strcpy(hireds[j].name, pt);
			i++;
		}else if(i == 1){
			hireds[j].quality = atoi(pt);
			i++;
		}else if(i == 2){
			hireds[j].speed = atoi(pt);
			i++;
		}else if(i == 3){
			hireds[j].price = atoi(pt);
			hireds[j].busy = 0;
			j++;
			i = 0;
		}		
		pt = strtok(NULL, check);
	}
	return amount;
}

int findProperStudent(char criteria, int amount, int money){
	int i = 0;
	int min = -1;
	int j = 0;
	for(i = 0; i < amount; i++){
		if(hireds[i].busy == 0 && money >= hireds[i].price){
			min = i;
			j = i + 1;
			i = amount;
		}
	}
	if(min == -1){
		return -1;
	}
	for(i = j; i < amount; i++){
		if(hireds[i].busy == 0 && money >= hireds[i].price){
			if(criteria == 'C'){
				if(hireds[min].price > hireds[i].price){
					min = i;
				}
			}else if(criteria == 'S'){
				if(hireds[min].speed < hireds[i].speed){
					min = i;
				}
			}else if(criteria == 'Q'){
				if(hireds[min].quality < hireds[i].quality){
					min = i;
				}
			}
		}
	}
	return min;
}