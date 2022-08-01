#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h> 
#include<errno.h>
#include<semaphore.h>
#include<signal.h>
#include<pthread.h>
#include "queue.h"
#include "helper.h"

int money = 0;
int done = 0;

Queue *homeworks;
Hired *hireds;
int *solveCounts;

sem_t full;
sem_t busy;
sem_t queueLock;
sem_t hiredLock;
sem_t wait_h;
sem_t *wait_student;

sig_atomic_t ctrl_c = 0;
struct sigaction sigIntHandler;

void ctrlc_handler(){
	ctrl_c = 1;
}

void* h_thread(void *arg){
	pthread_detach(pthread_self());

	int fd = *((int *)arg);
	char c[1];
	int size;

	while(1){
		if(done == 1 || money <= 0){
			if(ctrl_c == 0){
				printf("H has no more money for homeworks, terminating.\n");
			}
			sem_post(&wait_h);
			sem_post(&full);
			return(NULL);
		}
		size = read(fd, c, 1);
		if(size == -1){
			perror("read file");
			exit(EXIT_FAILURE);
		}else if(size == 0 || c[0] == '\n'){
			printf("H has no other homeworks, terminating.\n");
			sem_post(&wait_h);
			sem_post(&full);
			return(NULL);
		}
		sem_wait(&queueLock);
		enQueue(homeworks, c[0]);
		sem_post(&queueLock);
		sem_post(&full);
		printf("H has new homework %c; remaining money is %dTL\n", c[0], money);
	}
}

void* student_thread(void *arg){
	int index = *((int *)arg);
	printf("%s is waiting for a homework\n", hireds[index].name);
	while(1){
		sem_wait(&wait_student[index]);
		if(done == 1 && hireds[index].busy == 0){
			return(NULL);
		}
		sleep(6 - hireds[index].speed);
		sem_wait(&hiredLock);
		hireds[index].busy = 0;
		solveCounts[index] += 1;
		sem_post(&hiredLock);
		printf("%s is waiting for a homework\n", hireds[index].name);
		sem_post(&busy);	
	}
}

int main(int argc, char **argv){

	char *hwFile;
	char *studentFile;
	int amount;
	int fd;
	int hwFd;
	int i;
	pthread_t hw_ptid;
	int hwDone = 0;
	char c;
	int choosen;

	if(argc != 4){
		fprintf(stderr, "You must pass 3 command line arguments as {homeworksFilePath} {studentsFilePath} {money}\n");
		fprintf(stderr, "For example: ./program homeworks students 20\n");
		exit(EXIT_FAILURE);
	}

	hwFile = argv[1];
	studentFile = argv[2];
	money = atoi(argv[3]);

	fd = open(studentFile, O_RDONLY);
	if(fd == -1){
		perror("open file");
		exit(EXIT_FAILURE);
	}

	hwFd = open(hwFile, O_RDONLY);
	if(hwFd == -1){
		perror("open file");
		exit(EXIT_FAILURE);
	}

	sigIntHandler.sa_handler = &ctrlc_handler;
   	sigIntHandler.sa_flags = 0;
   	if((sigemptyset(&sigIntHandler.sa_mask) == -1) || (sigaction(SIGINT, &sigIntHandler, NULL) == -1)){
   		perror("Failed to install SIGINT signal handler");
   	}

	amount = fillHireds(fd);
	if(close(fd) == -1){
		perror("close");
		exit(EXIT_FAILURE);
	}
	solveCounts = calloc(amount, sizeof(int));
	if(solveCounts == NULL){
		fprintf(stderr, "Calloc is failed to allocate memory.\n");
	}

	printf("%d students-for-hire threads have been created.\n", amount);
	printf("Name Q S C\n");
	for(i = 0; i < amount; i++){
		printf("%s %d %d %d\n", hireds[i].name, hireds[i].quality, hireds[i].speed, hireds[i].price);
	}

	if(sem_init(&wait_h, 0, 0) == -1){
		perror("sem_init");
		exit(EXIT_FAILURE);
	}
	if(sem_init(&full, 0, 0) == -1){
		perror("sem_init");
		exit(EXIT_FAILURE);
	}
	if(sem_init(&busy, 0, amount) == -1){
		perror("sem_init");
		exit(EXIT_FAILURE);
	}
	if(sem_init(&queueLock, 0, 1) == -1){
		perror("sem_init");
		exit(EXIT_FAILURE);
	}
	if(sem_init(&hiredLock, 0, 1) == -1){
		perror("sem_init");
		exit(EXIT_FAILURE);
	}

	homeworks = createQueue();

	wait_student = malloc(sizeof(sem_t) * amount);
	if(wait_student == NULL){
		fprintf(stderr, "Malloc is failed to allocate memory.\n");
	}
	for(i = 0; i < amount; i++){
		if(sem_init(&wait_student[i], 0, 0) == -1){
			perror("sem_init");
			exit(EXIT_FAILURE);
		}
	}
	pthread_t student_ptids[amount];
	int indexes[amount];

	if(pthread_create(&hw_ptid, NULL, &h_thread, (void *)&hwFd) != 0){
		fprintf(stderr, "h thread could not be created.\n");
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < amount; i++){
		indexes[i] = i;
		if(pthread_create(&student_ptids[i], NULL, &student_thread, (void *)&indexes[i]) != 0){
			fprintf(stderr, "student-for-hire thread could not be created.\n");
			exit(EXIT_FAILURE);
		}
	} 
	
	while(1){
		if(ctrl_c == 1){
			printf("Termination signal received, closing.\n");
			done = 1;
			for(i = 0; i < amount; i++){
				sem_post(&wait_student[i]);
			}
			break;
		}
		sem_wait(&full);
		sem_wait(&queueLock);
		if(isEmpty(homeworks) == 1){
			printf("No more homeworks left or coming in, closing.\n");
			done = 1;
			for(i = 0; i < amount; i++){
				sem_post(&wait_student[i]);
			}
			break;
		} 
		c = homeworks->front->data;
		deQueue(homeworks);
		sem_post(&queueLock);
		sem_wait(&busy);
		if(ctrl_c == 1){
			printf("Termination signal received, closing.\n");
			done = 1;
			for(i = 0; i < amount; i++){
				sem_post(&wait_student[i]);
			}
			break;
		}
		sem_wait(&hiredLock);
		choosen = findProperStudent(c, amount, money);
		if(choosen == -1){
			printf("Money is over, closing.\n");
			done = 1;
			sem_post(&hiredLock);
			for(i = 0; i < amount; i++){
				sem_post(&wait_student[i]);
			}
			break;
		}
		hireds[choosen].busy = 1;
		money -= hireds[choosen].price;
		printf("%s is solving homework %c for %d, H has %d left.\n", hireds[choosen].name, c, hireds[choosen].price, money);
		sem_post(&hiredLock);
		sem_post(&wait_student[choosen]);
	}

	for(i = 0; i < amount; i++){
		pthread_join(student_ptids[i], NULL);
	}

	sem_wait(&wait_h);
	if(close(hwFd) == -1){
		perror("close");
		exit(EXIT_FAILURE);
	}
	empty(homeworks);
	free(homeworks);

	if(sem_destroy(&wait_h) == -1){
		perror("sem_destroy");
		exit(EXIT_FAILURE);
	}
	if(sem_destroy(&full) == -1){
		perror("sem_destroy");
		exit(EXIT_FAILURE);
	}
	if(sem_destroy(&busy) == -1){
		perror("sem_destroy");
		exit(EXIT_FAILURE);
	}
	if(sem_destroy(&queueLock) == -1){
		perror("sem_destroy");
		exit(EXIT_FAILURE);
	} 
	if(sem_destroy(&hiredLock) == -1){
		perror("sem_destroy");
		exit(EXIT_FAILURE);
	}
	for(i = 0; i < amount; i++){
		if(sem_destroy(&wait_student[i]) == -1){
			perror("sem_destroy");
			exit(EXIT_FAILURE);
		}
	}

	free(wait_student);

	printf("Homeworks solved and money made by the students:\n");
	for(i = 0; i < amount; i++){
		hwDone += solveCounts[i];
		printf("%s %d %d\n", hireds[i].name, solveCounts[i], solveCounts[i] * hireds[i].price);
	}

	for(i = 0; i < amount; i++){
		free(hireds[i].name);
	}
	free(hireds);
	free(solveCounts);

	printf("Total cost for %d homeworks %dTL\n", hwDone, atoi(argv[3]) - money);
	printf("Money left at H's account: %dTL\n", money);
	
	exit(EXIT_SUCCESS);
}

