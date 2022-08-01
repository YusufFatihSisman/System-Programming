#include<stdio.h>
#include<ctype.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>
#include<math.h>

sig_atomic_t closed_childs = 0;
sig_atomic_t child_exit_status = 0;
struct sigaction old_ctrlc_action;
int fdHold = 0;

void clean_up_child(){
	int status;
	wait(&status);
	child_exit_status = status;
}

void ctrlc_handler(){
	puts("ctrlc is catched");
	close(fdHold);
	int status;
	int i = 0;
	//for(i = 0; i < 8; i++){
//		wait(&status);
//	}
	sigaction(SIGINT, &old_ctrlc_action, NULL);
	kill(-1, SIGINT);
}

void child_handler(){
	closed_childs ++;
}

void parent_handler(){

}

void readLine(double x[8], double y[8], int line, int fd);
void writeLineEnd(char *message, int line, int fd);
double interpolate(double x[], double y[], double xi, int n);
double readAndAverage(int degree, int fd);
void PrintCoefficients(double x[], double y[]);

int main(int argc, char **argv){

	int fd = 0;
	int i = 0;
	struct sigaction sigIntHandler;
	struct sigaction childHandler; 
	struct sigaction parentHandler;
	struct sigaction sigChildAction;
	sigset_t prevMask, intMask;
	sigset_t childPrevMask, childIntMask;
	int ids[8] = {0};

	sigChildAction.sa_handler = &clean_up_child;
   	sigChildAction.sa_flags = 0;
   	if((sigemptyset(&sigChildAction.sa_mask) == -1) || (sigaction(SIGCHLD, &sigChildAction, NULL) == -1)){
   		perror("Failed to install SIGCHLD signal handler");
   	}

	sigIntHandler.sa_handler = &ctrlc_handler;
   	sigIntHandler.sa_flags = SA_RESETHAND;
   	if((sigemptyset(&sigIntHandler.sa_mask) == -1) || (sigaction(SIGINT, &sigIntHandler, NULL) == -1)){
   		perror("Failed to install SIGINT signal handler");
   	}

   	childHandler.sa_handler = &child_handler;
   	childHandler.sa_flags = 0;
   	if((sigemptyset(&childHandler.sa_mask) == -1) || (sigaction(SIGUSR1, &childHandler, NULL) == -1)){
   		perror("Failed to install SIGUSR1 signal handler");
   	}

   	parentHandler.sa_handler = &parent_handler;
   	parentHandler.sa_flags = 0;
   	if((sigemptyset(&parentHandler.sa_mask) == -1) || (sigaction(SIGUSR2, &parentHandler, NULL) == -1)){
   		perror("Failed to install SIGUSR2 signal handler");
   	}

	sigemptyset(&intMask);
    sigaddset(&intMask, SIGUSR1);
    sigaddset(&intMask, SIGINT);
   	sigprocmask(SIG_BLOCK, &intMask, &prevMask);
   	
   	sigemptyset(&childIntMask);
    sigaddset(&childIntMask, SIGUSR2);
    sigaddset(&childIntMask, SIGINT);
   	sigprocmask(SIG_BLOCK, &childIntMask, &childPrevMask);
	
	if(argc != 2){
		perror("Not right amount of command line argument are passed.");
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if(fd == -1){
		perror("File could not oppened");
		return -1;
	}
	fdHold = fd;
	
	for(i = 0; i < 8; i++){
		pid_t child_pid = fork();
		switch(child_pid){
			case 0:
				sigsuspend(&childPrevMask);
				double x[8] = {0.0};
				double y[8] = {0.0};
				char output[50] = "";
				readLine(x, y, i, fd);				
				snprintf(output, 50, "%.1lf", interpolate(x, y, x[7], 6));
    			writeLineEnd(output, i+1, fd);
				kill(getppid(), SIGUSR1);
				sigsuspend(&childPrevMask);
				snprintf(output, 50, "%.1lf", interpolate(x, y, x[7], 7));
				writeLineEnd(output, i+1, fd);
				printf("Polynomial %d: ", i);
				PrintCoefficients(x, y);
				kill(getppid(), SIGUSR1);
				sigsuspend(&childPrevMask);
				exit(EXIT_SUCCESS);	
			case -1:
				printf("fork is failed");
				exit(EXIT_FAILURE);
			default:
				ids[i] = child_pid;
				break;
		}
	}

	kill(ids[closed_childs], SIGUSR2);

	while(closed_childs < 8){
		sigsuspend(&prevMask);
		kill(ids[closed_childs], SIGUSR2);
	}
	closed_childs = 0;

	printf("Error of polynomial of degree 5: %.1lf\n", readAndAverage(5, fd));

	kill(ids[closed_childs], SIGUSR2);

	while(closed_childs < 8){
		sigsuspend(&prevMask);
		kill(ids[closed_childs], SIGUSR2);
	}

	printf("Error of polynomial of degree 6: %.1lf\n", readAndAverage(6, fd));

	int finished = 0;
	kill(ids[finished], SIGUSR2);
	finished++;
	sigaddset(&intMask, SIGCHLD);
	while(finished < 8){
		sigsuspend(&prevMask);
		kill(ids[finished], SIGUSR2);
		finished++;
	}
	
	if(close(fd) == -1){
		perror("close");
	}
	exit(EXIT_SUCCESS);
}

double readAndAverage(int degree, int fd){
	int i = 0;
	char buffer[1024] = "";
	double array[18] = {0.0};
	char c[1] = "";
	int j = 0;
	double sum = 0.0;
	char *pt;
	int k = 0;

	if(lseek(fd, 0, SEEK_SET) == -1){
		perror("lseek");
	}
	
	while(i < 8){
		do{
			if(read(fd, c, 1) == -1){
				perror("read");
			}
			if(j < 1023){
				buffer[j] = c[0];
				j++;
			}	
		}while(c[0] != '\n');
		buffer[j] = '\0';
		j = 0;
		pt = strtok(buffer, ",");
		while(pt != NULL){
			sscanf(pt, "%lf", &array[k]);
			k++;
			if(degree == 5 && k == 17){
				sum += fabs(array[15] - array[16]);
			}else if(degree == 6 && k == 18){
				sum += fabs(array[15] - array[17]);
			}
			pt = strtok (NULL, ",");
		}
		k = 0;
		i++;
	}
    return sum / 8;
}

void readLine(double x[8], double y[8], int line, int fd){
	char buffer[100] = "";
	char c[1] = "";
	int i = 0;
	char *pt;
	int fillX = 1;

	//go start of file
	if(lseek(fd, 0, SEEK_SET) == -1){
		perror("lseek");
	}
	i = 0;
	//arrive wanted location
	while(i < line){
		do{
			if(read(fd, c, 1) == -1){
				perror("read");
			}
		}while(c[0] != '\n');
		i++; 
	}
	i=0;
	if(read(fd, c, 1) == -1){
		perror("read");
	}
	while(c[0] != '\n'){
		if(i < 99){
			buffer[i] = c[0];
			i++;
		}
		if(read(fd, c, 1) == -1){
			perror("read");
		}
	}
	buffer[i] = '\0';
	i = 0;
	pt = strtok (buffer, ",");
    while (pt != NULL) {
    	if(fillX == 1){
    		sscanf(pt, "%lf", &x[i]);
    		fillX = 0;
    	}else{
    		sscanf(pt, "%lf", &y[i]);
    		fillX = 1;
    		i++;
    	}
        pt = strtok (NULL, ",");
    }
}

void writeLineEnd(char *message, int line, int fd){
	char buffer[1024] = "";
	char c[1] = "";
	int i = 0;
	char *newMes = malloc(strlen(message) + 3);
	strcpy(newMes, ",");
	strcat(newMes, message);
	strcat(newMes, "\n");

	//go start of file
	if(lseek(fd, 0, SEEK_SET) == -1){
		perror("lseek");
	}
	//arrive wanted location
	while(i < line){
		do{
			if(read(fd, c, 1) == -1){
				perror("read");
			}
		}while(c[0] != '\n');
		i++; 
	}
	
	//fill buffer with rest of file
	i = 0;
	while(read(fd, c, 1) > 0){
		buffer[i] = c[0];
		i++;
	}
	buffer[i] = '\0';

	//go start of file
	if(lseek(fd, 0, SEEK_SET) == -1){
	}
	i = 0;
	//arrive wanted location
	while(i < line){
		do{
			if(read(fd, c, 1) == -1){
				perror("read");
			}
		}while(c[0] != '\n');
		i++; 
	}
	//go before newline
	if(lseek(fd, -1, SEEK_CUR) == -1){
		perror("lseek");
	}
	//write new value
	if(write(fd, newMes, strlen(newMes)) == -1){
		perror("write");
	}
	//write rest of file
	if(write(fd, buffer, strlen(buffer)) == -1){
		perror("write");
	}
	free(newMes);
}

double interpolate(double x[], double y[], double xi, int n){
    double result = 0; 
    int i = 0; 
    int j = 0;
    for (i = 0; i < n; i++){
        double term = y[i];
        for (j = 0; j < n; j++){
            if (j != i){
                term = term*(xi - x[j])/(x[i] - x[j]);
            }
        }
        result += term;
    }
    return result;
}

void PrintCoefficients(double x[], double y[]){
	int i = 0;
	double coefficients[7] = {0.0};

	for(i = 0; i < 7; i++){
		coefficients[i] = 0;
	}
		
	for(i = 0; i < 7; i++){
	    double newCoefficients[8] = {0.0};
	    int j = 0;
	    if(i > 0){
	        newCoefficients[0] = -x[0]/(x[i]-x[0]);
	        newCoefficients[1] = 1/(x[i]-x[0]);
	    }else{
	        newCoefficients[0] = -x[1]/(x[i]-x[1]);
	        newCoefficients[1] = 1/(x[i]-x[1]);
	    }
	    int startIndex = 1; 
	    if(i == 0) {
	    	startIndex = 2; 
	    }
	    int n = 0;
	    for(n = startIndex; n < 7; n++){
	        if(i == n){
	        	continue;
	        } 
	        for(j = 6; j >= 1; j--){
	            newCoefficients[j] = newCoefficients[j]*(-x[n]/(x[i]-x[n]))+newCoefficients[j-1]/(x[i]-x[n]);
	        }
	        newCoefficients[0] = newCoefficients[0]*(-x[n]/(x[i]-x[n]));
	    }    
	    for(j = 0; j < 7; j++){
	    	coefficients[j] += y[i]*newCoefficients[j];
	    }
	}
	for(i = 0; i < 7; i++){
		printf("%.1lf,", coefficients[i]);
	}
	printf("\n");
}