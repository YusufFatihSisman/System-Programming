#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<fcntl.h> 
#include<errno.h>
#include<semaphore.h>
#include<signal.h>

void getArgs(int argc, char **argv, int *nurses, int *vaccinators, int *citizens, int *bufferSize, int *doses, char **path);


typedef struct Citizens{
	pid_t pid;
	int lastDose;
}Citizen;

sig_atomic_t child_exit_status = 0;
struct sigaction sigIntHandler;
struct sigaction citizenHandler;
sigset_t prevMask, intMask;

sem_t *empty;
sem_t *memLock;
sem_t *ready1;
sem_t *ready2;
sem_t *vaccinLock;
sem_t *citizenMemLock;
sem_t *vaccinateDone;

int fd;
int sfd;
char *buffer;
int cfd;
Citizen *citizenBuffer;
int vfd;
Citizen *vaccinatorBuffer;

void citizenInvite(){

}

void ctrlc_handler(){
	int status = 0;
	pid_t pid;
	do{
		pid = wait(&status);
	}while(pid != -1);
	sigIntHandler.sa_handler = SIG_DFL;
	sigIntHandler.sa_flags = 0;
   	if((sigemptyset(&sigIntHandler.sa_mask) == -1) || (sigaction(SIGINT, &sigIntHandler, NULL) == -1)){
   		perror("Failed to install SIGINT signal handler");
   	}
   	sem_close(empty);
 	sem_close(memLock);
 	sem_close(ready1);
 	sem_close(ready2);
 	sem_close(vaccinLock);
 	sem_close(citizenMemLock);
 	sem_close(vaccinateDone);

 	sem_unlink("empty");
 	sem_unlink("memLock");
 	sem_unlink("ready1");
 	sem_unlink("ready2");
 	sem_unlink("vaccinLock");
 	sem_unlink("citizenMemLock");
 	sem_unlink("vaccinateDone");

 	close(fd);
 	close(sfd);
 	close(vfd);

	munmap(buffer, sizeof(buffer));
	shm_unlink("buffer");

	munmap(citizenBuffer, sizeof(citizenBuffer));
	shm_unlink("citizenBuffer");

	munmap(vaccinatorBuffer, sizeof(vaccinatorBuffer));
	shm_unlink("vaccinatorBuffer");

   	kill(getpid(), SIGINT);
}


int main(int argc, char **argv){

	int nurses = 0;
	int vaccinators = 0;
	int citizens = 0;
	int bufferSize = 0;
	int doses = 0;
	char *path = NULL;
	int i = 0;
	struct flock lock;

	getArgs(argc, argv, &nurses, &vaccinators, &citizens, &bufferSize, &doses, &path);

	sigIntHandler.sa_handler = &ctrlc_handler;
   	sigIntHandler.sa_flags = 0;
   	if((sigemptyset(&sigIntHandler.sa_mask) == -1) || (sigaction(SIGINT, &sigIntHandler, NULL) == -1)){
   		perror("Failed to install SIGINT signal handler");
   	}

	citizenHandler.sa_handler = &citizenInvite;
   	citizenHandler.sa_flags = 0;
   	if((sigemptyset(&citizenHandler.sa_mask) == -1) || (sigaction(SIGUSR1, &citizenHandler, NULL) == -1)){
   		perror("Failed to install SIGUSR1 signal handler");
   	}

	sigemptyset(&intMask);
    sigaddset(&intMask, SIGUSR1);
    sigaddset(&intMask, SIGINT);
   	sigprocmask(SIG_BLOCK, &intMask, &prevMask);
	
	fd = open(path, O_RDWR);
	if(fd == -1){
		perror("open file");
		exit(EXIT_FAILURE);
	}

	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_RDLCK;
	
	sfd = shm_open("buffer", O_CREAT | O_RDWR, 0644);
	if(sfd == -1){
		perror("shm_oepn");
		exit(EXIT_FAILURE);
	}
	if(ftruncate(sfd, bufferSize) == -1){
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}
 	buffer = mmap(NULL, bufferSize , PROT_READ | PROT_WRITE, MAP_SHARED, sfd, 0);
 	close(sfd);

 	cfd = shm_open("citizenBuffer", O_CREAT | O_RDWR, 0644);
 	if(cfd == -1){
		perror("shm_oepn");
		exit(EXIT_FAILURE);
	}
	if(ftruncate(cfd, citizens * sizeof(Citizen)) == -1){
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}
	citizenBuffer = mmap(NULL, citizens * sizeof(Citizen) , PROT_READ | PROT_WRITE, MAP_SHARED, cfd, 0);
	close(cfd);

	vfd = shm_open("vaccinatorBuffer", O_CREAT | O_RDWR, 0644);
	if(vfd == -1){
		perror("shm_oepn");
		exit(EXIT_FAILURE);
	}
	if(ftruncate(vfd, vaccinators * sizeof(Citizen)) == -1){
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}
	vaccinatorBuffer = mmap(NULL, vaccinators * sizeof(Citizen) , PROT_READ | PROT_WRITE, MAP_SHARED, vfd, 0);
	close(vfd);

 	for(i = 0; i < bufferSize; i++){
 		buffer[i] = '0';
 	}

 	empty = sem_open("empty", O_CREAT, 0644, bufferSize);
 	memLock = sem_open("memLock", O_CREAT, 0644, 1);
 	ready1 = sem_open("ready1", O_CREAT, 0644, 0);
 	ready2 = sem_open("ready2", O_CREAT, 0644, 0);
 	vaccinLock = sem_open("vaccinLock", O_CREAT, 0644, 0);
 	citizenMemLock = sem_open("citizenMemLock", O_CREAT, 0644, 1);
 	vaccinateDone = sem_open("vaccinateDone", O_CREAT, 0644, 0);
	
 	printf("Welcome to the GTU344 clinic, Number of citizens to vaccinate c = %d, with t = %d doses.\n", citizens, doses);
 	int j = 0;
 	int ids[nurses + vaccinators + citizens];
 	for(i = 0; i < nurses + vaccinators + citizens; i++){
 		pid_t child_pid = fork();
 		switch(child_pid){
 			case 0:
 				if(i < citizens){
 					int j = 0;
 					for(j = 0; j < doses; j++){
 						sigsuspend(&prevMask);
 					}
 					sem_close(empty);
				 	sem_close(memLock);
				 	sem_close(ready1);
				 	sem_close(ready2);
				 	sem_close(vaccinLock);
				 	sem_close(citizenMemLock);
				 	sem_close(vaccinateDone);
 					exit(EXIT_SUCCESS);
 				}else if(i < citizens + nurses){
 					while(1){
 						//read 1 char from file
 						fcntl(fd, F_SETLKW, &lock);
	 					char rc[1];
	 					int size = read(fd, rc, 1);
	 					lock.l_type = F_UNLCK;
						fcntl(fd, F_SETLKW, &lock);
						//check char
	 					if(size == -1){
	 						perror("read");
	 						exit(EXIT_FAILURE);
	 					}else if(size == 0 || rc[0] == '\n'){
	 						printf("Nurses have carried all vaccines to the buffer, terminating.\n");
	 						sem_close(empty);
						 	sem_close(memLock);
						 	sem_close(ready1);
						 	sem_close(ready2);
						 	sem_close(vaccinLock);
						 	sem_close(citizenMemLock);
						 	sem_close(vaccinateDone);
	 						exit(EXIT_SUCCESS);
	 					}else{
	 						sem_wait(empty);
	 						sem_wait(memLock);
	 						int j = 0;
	 						//set new vaccine to buffer
	 						for(j = 0; j < bufferSize; j++){
	 							if(buffer[j] == '0'){
	 								buffer[j] = rc[0];
	 								j = bufferSize;
	 							}
	 						}
	 						int vac1 = 0;
	 						int vac2 = 0;
	 						//control current amount of vaccines
	 						for(j = 0; j < bufferSize; j++){
	 							if(buffer[j] == '1'){
	 								vac1++;
	 							}else if(buffer[j] == '2'){
	 								vac2++;
	 							}
	 						}
	 						printf("Nurse %d (pid = %d) has borught vaccine %c: the clinic has %d vaccine1 and %d vaccine2.\n",
	 							i - citizens + 1, getpid(), rc[0], vac1, vac2);
	 						sem_post(memLock);
	 						if(rc[0] == '1'){
	 							sem_post(ready1);
	 						}else{
	 							sem_post(ready2);
	 						}
	 					}
 					}
 				}else{
 					while(1){
 						sem_wait(vaccinLock);
 						sem_wait(citizenMemLock);
 						int remain = -1;
 						int j = 0;
 						//check if any citizens who does not get all the doses
 						for(j = 0; j < citizens; j++){
 							if(citizenBuffer[j].lastDose < doses){
 								remain = j;
 								j = citizens;
 							}
 						}
 						//if all citizens get full doses
 						if(remain == -1){
 							sem_post(vaccinLock);
 							sem_post(citizenMemLock);
 							sem_post(vaccinateDone);
 							sem_close(empty);
						 	sem_close(memLock);
						 	sem_close(ready1);
						 	sem_close(ready2);
						 	sem_close(vaccinLock);
						 	sem_close(citizenMemLock);
						 	sem_close(vaccinateDone);
 							exit(EXIT_SUCCESS);
 						}
	 					sem_wait(ready1);
	 					sem_wait(ready2);
	 					sem_post(vaccinLock);
	 					int nextCitizen = remain;
	 					int vaccinateTime = 0;
	 					int vaccinatedCitizen = 0;
	 					//set next citizen to be vaccinate in order
	 					for(j = nextCitizen; j < citizens - 1; j++){
	 						if(citizenBuffer[j+1].pid > citizenBuffer[nextCitizen].pid){ // next one is younger
	 							if(citizenBuffer[j+1].lastDose < citizenBuffer[nextCitizen].lastDose){ // next one has less dose
	 								nextCitizen = j + 1;
	 								j = citizens;
	 							}
	 						}	
	 					}
	 					//increase amount of how much dose vaccinator vaccinate
	 					for(j = 0; j < vaccinators; j++){
	 						if(vaccinatorBuffer[j].pid == getpid()){
	 							vaccinatorBuffer[j].lastDose += 1;
	 							j = vaccinators;
	 						}
	 					}
	 					vaccinatedCitizen = citizenBuffer[nextCitizen].pid;
	 					citizenBuffer[nextCitizen].lastDose += 1;
	 					vaccinateTime = citizenBuffer[nextCitizen].lastDose;
	 					printf("Vaccinator %d (pid=%d) is inviting citizen pid=%d to the clinic\n",
	 						i - nurses - citizens + 1, getpid(), vaccinatedCitizen);
	 					sem_post(citizenMemLock);
	 					kill(citizenBuffer[nextCitizen].pid, SIGUSR1);
	 					sem_wait(memLock);
	 					j = bufferSize - 1;
	 					int find1 = 0;
	 					int find2 = 0;
	 					//remove vaccinates from buffer
	 					for(j = bufferSize - 1; j >= 0; j--){
	 						if(buffer[j] == '1' && find1 == 0){
	 							buffer[j] = '0';
	 							find1++;
	 						}else if(buffer[j] == '2' && find2 == 0){
	 							buffer[j] = '0';
	 							find2++;
	 						}
	 						if(find1 == 1 && find2 == 1){
	 							j = -1;
	 						}
	 					}
	 					int vac1 = 0;
	 					int vac2 = 0;
	 					for(j = 0; j < bufferSize; j++){
	 						if(buffer[j] == '1'){
	 							vac1++;
	 						}else if(buffer[j] == '2'){
	 							vac2++;
	 						}
	 					}
	 					printf("Citizen %d (pid=%d) is vaccinated for the %d. time: the clinic has %d vaccine1 and %d vaccine2.\n", 
	 						nextCitizen + 1, vaccinatedCitizen, vaccinateTime, vac1, vac2);
	 					sem_post(memLock);
	 					sem_post(empty);
	 					sem_post(empty);
 					}
 				}
 			case -1:
 				perror("fork");
 				exit(EXIT_FAILURE);
 			default:
 				ids[i] = child_pid;
 				if(i < citizens){
 					citizenBuffer[i].pid = child_pid;
 					citizenBuffer[i].lastDose = 0;
 				}
 				if(i >= citizens + nurses && i < citizens + nurses + vaccinators){
 					vaccinatorBuffer[j].pid = child_pid;
 					vaccinatorBuffer[j].lastDose = 0;
 					j++;
 				}
 				break;
 		}
 	}
 	sem_post(vaccinLock);

 	for(i = 0; i < vaccinators; i++){
 		sem_wait(vaccinateDone);
 	}

 	printf("All citizens have been vaccinated.\n");
 	for(i = 0; i < vaccinators; i++){
 		printf("Vaccinator %d (pid=%d) vaccinated %d doses. ", 
 			i + 1, vaccinatorBuffer[i].pid, vaccinatorBuffer[i].lastDose);
 	}
 	printf("The clinic is now closed. Stay healthy.\n");
 	
 	/* CTRLC HANDLER CHECK
 	sigsuspend(&prevMask);
	*/

 	sem_close(empty);
 	sem_close(memLock);
 	sem_close(ready1);
 	sem_close(ready2);
 	sem_close(vaccinLock);
 	sem_close(citizenMemLock);
 	sem_close(vaccinateDone);

 	sem_unlink("empty");
 	sem_unlink("memLock");
 	sem_unlink("ready1");
 	sem_unlink("ready2");
 	sem_unlink("vaccinLock");
 	sem_unlink("citizenMemLock");
 	sem_unlink("vaccinateDone");

	munmap(buffer, bufferSize);
	shm_unlink("buffer");

	munmap(citizenBuffer, citizens * sizeof(Citizen));
	shm_unlink("citizenBuffer");

	munmap(vaccinatorBuffer, vaccinators * sizeof(Citizen));
	shm_unlink("vaccinatorBuffer");

	/* CTRLC HANDLER CHECK
 	sigsuspend(&prevMask);
	*/
	exit(EXIT_SUCCESS);
}	

void printInfo(){
	printf("Proper way of execute this program:\n");
	printf("./program -n 3 -v 2 -c 3 -b 11 -t 3 -i inputfilepath\n");
	printf("n >= 2, v >= 2, c >= 3, b >= (t * c) + 1, t >= 1\n");
}

void getArgs(int argc, char **argv, int *nurses, int *vaccinators, int *citizens, int *bufferSize, int *doses, char **path){
	
	int c = 0;

	while((c = getopt(argc, argv, "n:v:c:b:t:i:")) != -1){
		switch(c){
			case 'n':
				*nurses = atoi(optarg);
				break;
			case 'v':
				*vaccinators = atoi(optarg);
				break;
			case 'c':
				*citizens = atoi(optarg);
				break;
			case 'b':
				*bufferSize = atoi(optarg);
				break;
			case 't':
				*doses = atoi(optarg);
				break;
			case 'i':
				*path = optarg;
				break;
			case '?':
				if(optopt == 'n' || optopt == 'v' 
					|| optopt == 'v' || optopt == 'b'
					|| optopt == 't' || optopt == 'i'){
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
	if(*nurses < 2){
		printf("Amount of nurses must be greater than 1\n");
		printInfo();
		exit(EXIT_FAILURE);
	}
	if(*vaccinators < 2){
		printf("Amount of vaccinators must be greater than 1\n");
		printInfo();
		exit(EXIT_FAILURE);
	}
	if(*citizens < 3){
		printf("Amount of citizens must be greater than 2\n");
		printInfo();
		exit(EXIT_FAILURE);
	}
	if(*bufferSize < (*citizens) * (*doses) + 1){
		printf("Amount of bufferSize must be greater than %d(number of citizens * required shots + 1)\n", (*citizens) * (*doses) + 1);
		printInfo();
		exit(EXIT_FAILURE);
	}
	if(*doses < 1){
		printf("Amount of citizens must be greater than 0\n");
		printInfo();
		exit(EXIT_FAILURE);
	}
}