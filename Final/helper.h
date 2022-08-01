#ifndef HELPER_H_INCLUDED
#define HELPER_H_INCLUDED

#define BD_NO_CHDIR 01
#define BD_NO_CLOSE_FILES 02
#define BD_NO_REOPEN_STD_FDS 04
#define BD_NO_UNMASK0 010
#define BD_MAX_CLOSE 8192

#define MAXLINELENGTH 1000

#define MAXCOMMANDLENGTH 300
#define STR_SELECT "SELECT"
#define STR_UPDATE "UPDATE TABLE SET"
#define STR_SELECT_DISTINCT "SELECT DISTINCT"
#define STR_FROM_TABLE "FROM TABLE"
#define STR_WHERE "WHERE"

enum OP{
	SELECT,
	DISTINCT,
	UPDATE,
};

struct Table{
	char ***memory;
	int sizeX;
	int sizeY;
	int capacityY;
};

extern struct Table table;

int becomeDaemon(int flags);
void freeTable();
void update(char **args, int argSize, char* whereArg, int socket_fd);
void getArgs(int argc, char **argv, int *port, char **logFilePath, int *poolSize, char **datasetPath);
void csvRead(char *path);
int parser(char* command, char ***args, char **whereArg, int* argsSize);
void beforeEquation(char *command, char *temp);
void afterEquation(char *command, char *temp);
int isWhere(char* command);
int isFromTable(char* command, int index);
int commandType(char *command, int idLength);
int isSelect(char *command);
int isDistinct(char *command);
int isUpdate(char *command);
void selectPrint(enum OP type, char **args, int argSize, int socket_fd);
int selectFunc(enum OP type, char **args, int argSize, int *arr);


#endif 
