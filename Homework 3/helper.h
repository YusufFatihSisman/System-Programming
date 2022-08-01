#ifndef HELPER_H_INCLUDED
#define HELPER_H_INCLUDED

typedef struct Hired{
	char *name;
	int quality;
	int speed;
	int price;
	int busy;
}Hired;

extern Hired *hireds;

int findProperStudent(char criteria, int amount, int money);
int fillHireds(int fd);

#endif