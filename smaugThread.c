#include <stdio.h>
#include <stdlib.h> 
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>


#include <curses.h>
#include <signal.h>
#include <sys/resource.h> 
#include <errno.h> 
#include <wait.h> 
#include <unistd.h>

struct myargs{
	int sleepingTime;
	int threadID;
};

/* Threads to create and control dragon, sheep, thiefs, 
   hunters and cows */
pthread_t cowThreadp[200];
pthread_t sheepThreadp[200];
pthread_t hunterThreadp[200];
pthread_t thiefThreadp[200];
pthread_t smaugThreadp;
pthread_t makeMoreSheepThreadp;
pthread_t makeMoreCowsThreadp;
pthread_t makeMoreHuntersThreadp;
pthread_t makeMoreThievesThreadp;
int beastThreadCounter = 0;
int hunterThreadCounter = 0;
int thiefThreadCounter = 0;


/* Define semaphores and mutexes */
/* mutexes for counter protection and the counters they protect */
pthread_mutex_t protectCowsInGroup;
int cowsInGroupCounter = 0;
pthread_mutex_t protectSheepInGroup;
int sheepInGroupCounter = 0;
pthread_mutex_t protectCowsWaiting;
int cowsWaitingCounter = 0;
pthread_mutex_t protectSheepWaiting;
int sheepWaitingCounter = 0;
pthread_mutex_t protectHuntersWaiting;
int huntersWaitingCounter = 0;
pthread_mutex_t protectThievesWaiting;
int thievesWaitingCounter = 0;
pthread_mutex_t protectCowsEaten;
int cowsEatenCounter = 0;
pthread_mutex_t protectSheepEaten;
int sheepEatenCounter = 0;
pthread_mutex_t protectMealWaitingFlag;
int mealWaitingFlag = 0;


/* Define semaphores to be placed in a single semaphore set */
/* Numbers indicate index in semaphore set for named semaphore */
sem_t sheepInGroup;
sem_t cowsInGroup;
sem_t sheepWaiting;
sem_t cowsWaiting;
sem_t huntersWaiting;
sem_t thievesWaiting;
sem_t sheepEaten;
sem_t cowsEaten;
sem_t huntersFought;
sem_t thievesPlayedWith;;
sem_t sheepDead;
sem_t cowsDead;
sem_t huntersDefeated;
sem_t thievesDefeated;;
sem_t dragonEating;
sem_t dragonFighting;
sem_t dragonPlaying;;
sem_t dragonSleeping;


/* Global variables to control the time between the arrivals */
/* of successive cows and sheep */
int terminationFlag = 0;
int mealWaiting = 0;
double maxCowIntervalUsec =  0.0;
double maxSheepIntervalUsec = 0.0;
double maxHunterIntervalUsec = 0.0;
double maxThiefIntervalUsec = 0.0;
double winProb = 0.0;
struct timeval startTime;

/* System constants used to control simulation termination */
#define COWS_IN_GROUP 1
#define SHEEP_IN_GROUP 3
#define MAX_SHEEP_EATEN 36
#define MAX_COWS_EATEN 12
#define MAX_HUNTERS_DEFEATED 48
#define MAX_THIEVES_DEFEATED 36
#define MAX_TREASURE_IN_HOARD 1000
#define INITIAL_TREASURE_IN_HOARD 500


/*  Pointers and ids for functions*/
int terminateFlag = 0;
void initialize();
void *smaug();
void *cow(void *inputStruct);
void *sheep(void *inputStruct);
void *hunter(void *inputStruct);
void *thief(void *inputStruct);
void *makeMoreCows();
void *makeMoreSheep();
void *makeMoreHunters();
void *makeMoreThieves();
int sem_waitChecked(sem_t *semaphoreID);
int sem_postChecked(sem_t *semaphoreID);
int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value);
int sem_destroyChecked(sem_t *semaphoreID);
int pthread_mutex_lockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_initChecked(pthread_mutex_t *mutexID,
		                              const pthread_mutexattr_t *attrib);
int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID);
void terminateSimulation();

int main( )
{
	double maxSheepInterval =  0.0;
	double maxCowInterval =  0.0;
	double maxHunterInterval =  0.0;
	double maxThiefInterval =  0.0;
	int newSeed = 0;

	/* initialize semaphores */
	initialize();


        /* obtain user input */
	printf("Please enter a random seed to start the simulation\n");
	scanf("%d",&newSeed);
	srand(newSeed);
	printf("Please enter the probability a hunter or thief is defeated (ms)");
	scanf("%lf", &winProb);
	printf("Please enter the maximum interval length for sheep (ms)");
	scanf("%lf", &maxSheepInterval);
	printf("Please enter the maximum interval length for cow (ms)");
	scanf("%lf", &maxCowInterval);
	printf("Please enter the maximum interval length for hunters (ms)");
	scanf("%lf", &maxHunterInterval);
	printf("Please enter the maximum interval length for thieves (ms)");
	scanf("%lf", &maxThiefInterval);
	maxSheepIntervalUsec = (int)maxSheepInterval * 1000;
	printf("max Sheep interval time %f \n", maxSheepInterval);
	maxCowIntervalUsec = (int)maxCowInterval * 1000;
	printf("max Cow interval time %f \n", maxCowInterval);
	maxHunterIntervalUsec = (int)maxHunterInterval * 1000;
	printf("max Hunterinterval time %f \n", maxHunterInterval);
	maxThiefIntervalUsec = (int)maxThiefInterval * 1000;
	printf("max Thief interval time %f \n", maxThiefInterval);


        /* create threads to make sheep, cows, hunters and thieves */
	pthread_create(&makeMoreSheepThreadp, NULL, makeMoreSheep, NULL);
	pthread_create(&makeMoreCowsThreadp, NULL, makeMoreCows, NULL);
	pthread_create(&makeMoreHuntersThreadp, NULL, makeMoreHunters, NULL);
	pthread_create(&makeMoreThievesThreadp, NULL, makeMoreThieves, NULL);
	pthread_create(&smaugThreadp, NULL, smaug, NULL);

        /*  wait till processes creating threads to make sheep, cows, 
            smaug, hunters, and thieves  return to the main program, then
            terminate them by joining then back to the original thread*/
	pthread_join(smaugThreadp, NULL);
	printf("smaug joined \n");
	pthread_join(makeMoreCowsThreadp, NULL);
	printf("cows joined \n");
	pthread_join(makeMoreSheepThreadp, NULL);
	printf("Sheep joined\n");
	pthread_join(makeMoreHuntersThreadp, NULL);
	printf("Hunters joined\n");
	pthread_join(makeMoreThievesThreadp, NULL);
	printf("Thieves joined\n");
	terminateSimulation();
	printf("Teminated final%f \n", maxCowInterval);
	exit(0);
}

	
void *makeMoreSheep()
{
	/* local counters, keep track of total number */
	/* of processes of each type created */
	int sheepCreated = 0;

	/* Variables to control the time between the arrivals */
	/* of successive beasts */
	int nextInterval = 0.0;
	int threadID;
	struct myargs *inSheepp;
	struct myargs inSheep;

	inSheepp = &inSheep;
	inSheepp->threadID = 3;
	printf("CRCRCRCRCRCRCRCRCRCRCR    make sheep process running \n");

	/* be sure you can put the pointer to your new sheep in the array of */
	/* pointers to sheep processes sheepThreadp */
	while( inSheepp->threadID < 200 && terminationFlag == 0) { 
		/* Create a sheep process */
		/* The condition used to determine if a process is needed is */
		/* if the last sheep created will be enchanted */
		nextInterval = (int)(((double)rand() / RAND_MAX) * maxSheepIntervalUsec);
		inSheepp->sleepingTime = (int)( ((double)rand() / RAND_MAX) * maxSheepIntervalUsec);
		threadID = inSheepp->threadID;
		inSheepp->threadID++;
		if(pthread_create((&sheepThreadp[sheepCreated]), NULL, sheep, (void *)inSheepp) == 0){
			sheepCreated++;
			printf("CRCRCR %8d CRCRCR    NEW SHEEP CREATED %d\n", threadID, sheepCreated);
		}
		else {
			printf("CRCRCRCRCRCRCRCRCRCRCR   sheep process not created \n");
			continue;
		}
		/* make sure we have not created more than the maximum allowed # of sheep */
		/* sleep until next thread needs to be created */
	        usleep(nextInterval);
	}

	terminateSimulation();
	printf("GOODBYE from thread makeMoreSheep\n" );
	fflush(stdout);
	exit(0);
}
	
void *makeMoreCows()
{
	/* local counters, keep track of total number */
	/* of processes of each type created */
	int cowsCreated = 0;

	/* Variables to control the time between the arrivals */
	/* of successive beasts */
	int nextInterval = 0;
	int threadID;
	struct myargs inCow;
	struct myargs *inCowp;

	inCowp = &inCow;
	inCowp->threadID = 3;
	printf("CRCRCRCRCRCRCRCRCRCRCR    make cows process running \n");

	/* be sure you can put the pointer to your new cow in the array of */
	/* pointers to sheep processes cowThreadp */
	while( inCowp->threadID < 200 && terminationFlag == 0) { 
		/* Create a cow process */
		nextInterval = (int)(((double)rand() / RAND_MAX) * maxCowIntervalUsec);
		inCowp->sleepingTime = (int)( ((double)rand() / RAND_MAX) * maxCowIntervalUsec);
		threadID = inCowp->threadID;
		inCowp->threadID++;
		if (pthread_create((&cowThreadp[cowsCreated]), NULL, cow, (void *)inCowp) == 0){
		cowsCreated++;
		printf("CRCRCR %8d CRCRCR    NEW COW CREATED %d\n", threadID, cowsCreated);
		}
		else {
			printf("CRCRCRCRCRCRCRCRCRCR  cow thread not created \n");
			continue;
		}
		/* make sure we have not created more than the maximum allowed # of cows */
		/* sleep until next thread needs to be created */
	        usleep( nextInterval);
	}
	terminateSimulation();
	printf("GOODBYE from thread makeMoreCows\n" );
	fflush(stdout);
	exit(0);
}

void *makeMoreHunters()
{
	/* local counters, keep track of total number */
	/* of processes of each type created */
	int huntersCreated = 0;

	/* Variables to control the time between the arrivals */
	/* of successive beasts */
	int nextInterval = 0;
	int threadID;
	struct myargs inHunter;
	struct myargs *inHunterp;

	inHunterp = &inHunter;
	inHunterp->threadID = 3;
	printf("CRCRCRCRCRCRCRCRCRCRCR    make hunters process running \n");

	/* be sure you can put the pointer to your new hunter in the array of */
	/* pointers to sheep processes hunterThreadp */
	while( inHunterp->threadID < 200 && terminationFlag == 0) { 
		/* Create a hunter process */
		nextInterval = (int)(((double)rand() / RAND_MAX) * maxHunterIntervalUsec);
		inHunterp->sleepingTime = (int)( ((double)rand() / RAND_MAX) * maxHunterIntervalUsec);
		threadID = inHunterp->threadID;
		inHunterp->threadID++;
		if (pthread_create((&hunterThreadp[huntersCreated]), NULL, hunter, (void *)inHunterp) == 0){
		huntersCreated++;
		printf("CRCRCR %8d CRCRCR    NEW hunter CREATED %d\n", threadID, huntersCreated);
		}
		else {
			printf("CRCRCRCRCRCRCRCRCRCR  hunter thread not created \n");
			continue;
		}
		/* make sure we have not created more than the maximum allowed # of hunters */
		/* sleep until next thread needs to be created */
	        usleep( nextInterval);
	}
	terminateSimulation();
	printf("GOODBYE from thread makeMoreHunters\n" );
	fflush(stdout);
	exit(0);
}

void *makeMoreThieves()
{
	/* local counters, keep track of total number */
	/* of processes of each type created */
	int thiefsCreated = 0;

	/* Variables to control the time between the arrivals */
	/* of successive beasts */
	int nextInterval = 0;
	int threadID;
	struct myargs inThief;
	struct myargs *inThiefp;

	inThiefp = &inThief;
	inThiefp->threadID = 3;
	printf("CRCRCRCRCRCRCRCRCRCRCR    make thiefs process running \n");

	/* be sure you can put the pointer to your new thief in the array of */
	/* pointers to sheep processes thiefThreadp */
	while( inThiefp->threadID < 200 && terminationFlag == 0) { 
		/* Create a thief process */
		nextInterval = (int)(((double)rand() / RAND_MAX) * maxThiefIntervalUsec);
		inThiefp->sleepingTime = (int)( ((double)rand() / RAND_MAX) * maxThiefIntervalUsec);
		threadID = inThiefp->threadID;
		inThiefp->threadID++;
		if (pthread_create((&thiefThreadp[thiefsCreated]), NULL, thief, (void *)inThiefp) == 0){
		thiefsCreated++;
		printf("CRCRCR %8d CRCRCR    NEW thief CREATED %d\n", threadID, thiefsCreated);
		}
		else {
			printf("CRCRCRCRCRCRCRCRCRCR  thief thread not created \n");
			continue;
		}
		/* make sure we have not created more than the maximum allowed # of thiefs */
		/* sleep until next thread needs to be created */
	        usleep( nextInterval);
	}
	terminateSimulation();
	printf("GOODBYE from thread makeMoreCows\n" );
	fflush(stdout);
	exit(0);
}


void *smaug( )
{
	int temp;
	int threadId;
	int k;


	/* local counters used only for smaug routine */
	int sheepEatenTotal = 0;
	int cowsEatenTotal = 0;
	int thievesDefeatedTotal = 0;
	int huntersDefeatedTotal = 0;
	int treasureC = 500;


	/* Initialize random number generator*/
	/* Random numbers are used to determine the time between successive beasts */
	threadId = 2;
	printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug is alive %u\n", threadId );
	printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has gone to sleep\n" );
	sem_waitChecked(&dragonSleeping);
	printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has woken up \n" );
	while (terminationFlag == 0) {
		pthread_mutex_lockChecked(&protectThievesWaiting);
		pthread_mutex_lockChecked(&protectHuntersWaiting);
		pthread_mutex_lockChecked(&protectMealWaitingFlag);
		if( mealWaitingFlag >= 1) {
			mealWaitingFlag -= 1;
			printf("SMAUGSMAUGSMAUGSMAUGSM    wait meal flag %d\n", mealWaitingFlag);
			pthread_mutex_unlockChecked(&protectMealWaitingFlag);
			pthread_mutex_unlockChecked(&protectHuntersWaiting);
			pthread_mutex_unlockChecked(&protectThievesWaiting);
			printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug is eating a meal\n");
			for( k = 0; k < SHEEP_IN_GROUP; k++ ) {
				sem_postChecked(&sheepWaiting);
				printf("SMAUGSMAUGSMAUGSMAUGSM    A sheep ready to eat\n");
			}
			sem_postChecked(&cowsWaiting);

			/*Smaug waits to eat*/
			sem_waitChecked(&dragonEating);
			for( k = 0; k < SHEEP_IN_GROUP; k++ ) {
				sem_postChecked(&sheepDead);
				sheepEatenTotal++;
				printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug finished eating a sheep\n");
			}
			sem_postChecked(&cowsDead);
			cowsEatenTotal++;
			printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug finished eating a cow\n");
			printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has finished a meal\n");
			if(sheepEatenTotal >= MAX_SHEEP_EATEN && cowsEatenTotal >= MAX_COWS_EATEN ) {
				printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has eaten the allowed number of cows and sheep\n");
				terminationFlag= 1;
				break; 
			}
			/*Smaug takes a nap*/
			printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug takes a nap\n");
			usleep(10);
		}
                else if(thievesWaitingCounter >= 1 ) {
			pthread_mutex_unlockChecked(&protectMealWaitingFlag);
			pthread_mutex_unlockChecked(&protectHuntersWaiting);
			thievesWaitingCounter--;
                        printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has discovered a thief in the valley %d \n", thievesWaitingCounter );
			sem_postChecked(&thievesWaiting);
			pthread_mutex_unlockChecked(&protectThievesWaiting);

			printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug lets the thief see him\n" );
			sem_waitChecked(&dragonPlaying);
                        printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug is playing with a thief\n");
                        if( (rand()%100) >  winProb ) {
                                printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has defeated a thief\n");
                                treasureC+=20;
                                printf("SMAUGSMAUGSMAUGSMAUGSM    horde contains %d jewels\n", treasureC);
                                printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has added to his treasure\n");
                                if( treasureC >= MAX_TREASURE_IN_HOARD ) {
                                        printf("SMAUGSMAUGSMAUGSMAUGSM     Smaugs horde is full\n");
                                        terminationFlag = 1;
                                        break;
                                }
                       }
                       else {
                                printf("SMAUGSMAUGSMAUGSMAUGSM    A thief has played well enough to be rewarded\n");
                                treasureC-=8;
                                printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has given away some treasure\n");
                                if(treasureC <= 0) {
                                        printf("SMAUGSMAUGSMAUGSMAUGSM    A thief receives ");
                                        printf("the rest of Smaug's horde\n     ");
                                        printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has no more treasure\n");
                                        terminateFlag = 1;
                                        break;
                                }
                                else {
                                        printf("SMAUGSMAUGSMAUGSMAUGSM     A thief receives treasure\n");
                                        printf("SMAUGSMAUGSMAUGSMAUGSM     Smaugs horde contains %d jewels\n", treasureC );
                                }
                        }
                        thievesDefeatedTotal++;
			sem_postChecked(&thievesDefeated);
                        printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has finished a game\n");
                        if(thievesDefeatedTotal >= MAX_THIEVES_DEFEATED ) {
                                printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has defeated the maximum number of thieves\n");
                                terminateFlag = 1;
                                break;
                        }

		}
                else if(huntersWaitingCounter >= 1 ) {
			pthread_mutex_unlockChecked(&protectMealWaitingFlag);
			huntersWaitingCounter--;
                        printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has discovered a thief in the valley %d \n", huntersWaitingCounter );
			sem_postChecked(&huntersWaiting);
			pthread_mutex_unlockChecked(&protectHuntersWaiting);
			pthread_mutex_unlockChecked(&protectThievesWaiting);

			printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug lets the hunter see him\n" );
			sem_waitChecked(&dragonFighting);
                        printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug is fighting with a hunter\n");
                        if( (rand()%100) >  winProb ) {
                                printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has defeated a hunter\n");
                                treasureC+=5;
                                printf("SMAUGSMAUGSMAUGSMAUGSM    horde contains %d jewels\n", treasureC);
                                printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has added to his treasure\n");
                                if( treasureC >= MAX_TREASURE_IN_HOARD ) {
                                        printf("SMAUGSMAUGSMAUGSMAUGSM     Smaugs horde is full\n");
                                        terminationFlag = 1;
                                        break;
                                }
                       }
                       else {
                                printf("SMAUGSMAUGSMAUGSMAUGSM    A hunter has fought well enough to be rewarded\n");
                                treasureC-=10;
                                printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has given away some treasure\n");
                                if(treasureC <= 0) {
                                        printf("SMAUGSMAUGSMAUGSMAUGSM    A hunter receives ");
                                        printf("the rest of Smaug's horde\n     ");
                                        printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has no more treasure\n");
                                        terminateFlag = 1;
					break;
                                }
                                else {
                                        printf("SMAUGSMAUGSMAUGSMAUGSM    A hunter receives treasure\n");
                                        printf("SMAUGSMAUGSMAUGSMAUGSM    Smaugs horde contains %d jewels\n", treasureC );
                                }
                        }
                        huntersDefeatedTotal++;
			sem_postChecked(&thievesDefeated);
                        printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has finished a battle\n");
                        if(huntersDefeatedTotal >= MAX_HUNTERS_DEFEATED ) {
                                printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug has defeated the maximum number of hunters\n");
                                terminateFlag = 1;
                                break;
                        }
                }
                else {
			pthread_mutex_unlockChecked(&protectMealWaitingFlag);
			pthread_mutex_unlockChecked(&protectHuntersWaiting);
			pthread_mutex_unlockChecked(&protectThievesWaiting);
                }
                if (terminationFlag == 1) break;
		printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug sleeps again\n");
		sem_waitChecked(&dragonSleeping);
		printf("SMAUGSMAUGSMAUGSMAUGSM    Smaug is awake again\n");
	}
	printf("SMAUGSMAUGSMAUGSMAUGSM    leaving loop\n");
	terminateSimulation();
	printf("GOODBYE from thread smaug\n" );
	fflush(stdout);
	exit(0);
}


void initialize()
{
	/* Init to zero, no elements are produced yet */
	sem_initChecked(&sheepInGroup, 0, 0);
	sem_initChecked(&cowsInGroup, 0, 0);
	sem_initChecked(&sheepWaiting, 0, 0);
	sem_initChecked(&cowsWaiting, 0, 0);
	sem_initChecked(&huntersWaiting, 0, 0);
	sem_initChecked(&thievesWaiting, 0, 0);
	sem_initChecked(&sheepEaten, 0, 0);
	sem_initChecked(&cowsEaten, 0, 0);
	sem_initChecked(&sheepDead, 0, 0);
	sem_initChecked(&cowsDead, 0, 0);
	sem_initChecked(&huntersDefeated, 0, 0);
	sem_initChecked(&thievesDefeated, 0, 0);
	sem_initChecked(&dragonSleeping, 0, 0);
	sem_initChecked(&dragonEating, 0, 0);
	sem_initChecked(&dragonPlaying, 0, 0);
	sem_initChecked(&dragonFighting, 0, 0);
	printf("!!INIT!!INIT!!INIT!!  semaphores initiialized\n");
	
	/* Init Mutex to one */
	pthread_mutex_initChecked(&protectSheepInGroup, NULL);
	pthread_mutex_initChecked(&protectCowsInGroup, NULL);
	pthread_mutex_initChecked(&protectMealWaitingFlag, NULL);
	pthread_mutex_initChecked(&protectHuntersWaiting, NULL);
	pthread_mutex_initChecked(&protectThievesWaiting, NULL);
	pthread_mutex_initChecked(&protectSheepEaten, NULL);
	pthread_mutex_initChecked(&protectCowsEaten, NULL);
	printf("!!INIT!!INIT!!INIT!!  mutexes initiialized\n");


	printf("!!INIT!!INIT!!INIT!!   initialize end\n");
}

void *sheep( void *inputStruct)
{
	int threadId;
	int startTimeN;
	int k;

        	
	threadId = ((struct myargs *)inputStruct)->threadID;
        startTimeN = ((struct myargs *)inputStruct)->sleepingTime; 

	/* graze */
	printf("SHSHSH %8d SHSHSH    A sheep is born\n", threadId);
	if( startTimeN > 0) {
		usleep( startTimeN );
	}
	printf("SHSHSH %8d SHSHSH    sheep grazes for %f ms\n", threadId, startTimeN/1000.0);


	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	pthread_mutex_lockChecked(&protectSheepInGroup);
	pthread_mutex_lockChecked(&protectCowsInGroup);
	sem_postChecked(&sheepInGroup);
	sheepInGroupCounter += 1;
	printf("SHSHSH %8d SHSHSH    %-2d sheep have been enchanted \n", threadId, sheepInGroupCounter);
	if( (sheepInGroupCounter >= SHEEP_IN_GROUP )&&
			( cowsInGroupCounter >= COWS_IN_GROUP )) {
		sem_waitChecked(&cowsInGroup);
		cowsInGroupCounter -= 1;
		pthread_mutex_unlockChecked(&protectCowsInGroup);
		printf("SHSHSH %8d SHSHSH    A cow is waiting\n", threadId);
		sheepInGroupCounter -= SHEEP_IN_GROUP;
		for (k=0; k<SHEEP_IN_GROUP; k++){
			sem_waitChecked(&sheepInGroup);
		}
		printf("SHSHSH %8d SHSHSH    The last sheep is waiting\n", threadId);
		pthread_mutex_unlockChecked(&protectSheepInGroup);
		pthread_mutex_lockChecked(&protectMealWaitingFlag);
		mealWaitingFlag += 1;
		printf("SHSHSH %8d SHSHSH    wait meal flag %d\n", threadId, mealWaitingFlag);
		pthread_mutex_unlockChecked(&protectMealWaitingFlag);
		printf("SHSHSH %8d SHSHSH    last sheep wakes the dragon \n", threadId);
		sem_postChecked(&dragonSleeping);
	}
	else
	{
		pthread_mutex_unlockChecked(&protectCowsInGroup);
		pthread_mutex_unlockChecked(&protectSheepInGroup);
	}

	sem_waitChecked(&sheepWaiting);

	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	printf("SHSHSH %8d SHSHSH    A sheep has woken up to be eaten\n", threadId);
	pthread_mutex_lockChecked(&protectSheepEaten);
	pthread_mutex_lockChecked(&protectCowsEaten);
	sem_postChecked(&sheepEaten);
	sheepEatenCounter += 1;
	if( ( sheepEatenCounter >= SHEEP_IN_GROUP ) &&
			( cowsEatenCounter >= COWS_IN_GROUP )) {
		cowsEatenCounter -= 1;
		printf("SHSHSH %8d SHSHSH    The cow has been eaten\n", threadId);
		pthread_mutex_unlockChecked(&protectCowsEaten);
		sem_waitChecked(&cowsEaten);
		sheepEatenCounter -= SHEEP_IN_GROUP;
		pthread_mutex_unlockChecked(&protectSheepEaten);
		for (k=0; k<SHEEP_IN_GROUP; k++){
			sem_waitChecked(&sheepEaten);
		}
		printf("SHSHSH %8d SHSHSH    The last sheep has been eaten\n", threadId);
		sem_postChecked(&dragonEating);
	}
	else
	{
		pthread_mutex_unlockChecked(&protectCowsEaten);
		pthread_mutex_unlockChecked(&protectSheepEaten);
		printf("SHSHSH %8d SHSHSH    A sheep has been eaten\n", threadId);
	}
	sem_waitChecked(&sheepDead);
	printf("SHSHSH %8d SHSHSH    sheep  dies\n", threadId);
	pthread_exit(NULL);
}


void *thief( void *inputStruct )
 {
	int threadID;
	int startTimeN;

        	
	threadID = ((struct myargs *)inputStruct)->threadID;
        startTimeN = ((struct myargs *)inputStruct)->sleepingTime; 

        printf("TTTTTT %8d TTTTTT    A thief is travelling to the valley\n", threadID);

         /* graze */
	if( startTimeN > 0) {
		usleep( startTimeN);
	}

        if( terminateFlag != 1) {
                printf("TTTTTT %8d TTTTTT    thief arrives after travelling for %lf ms\n", threadID, startTimeN/1000.0);
        }
 
	pthread_mutex_lockChecked(&protectThievesWaiting);
        thievesWaitingCounter = thievesWaitingCounter+1;
	pthread_mutex_unlockChecked(&protectThievesWaiting);
 
        printf("TTTTTT %8d TTTTTT    thief on the path wakes Smaug if he is sleeping\n", threadID );
	sem_postChecked(&dragonSleeping);
	sem_waitChecked(&thievesWaiting);
        printf("TTTTTT %8d TTTTTT    thief is playing with Smaug\n", threadID);
	sem_postChecked(&dragonPlaying);
        printf("TTTTTT %8d TTTTTT    The thief has been played with\n", threadID);
	sem_waitChecked(&thievesDefeated);
        printf("TTTTTT %8d TTTTTT    A thief leaves \n", threadID);
	pthread_exit(NULL);
 }


void *hunter( void *inputStruct )
 {
	int threadID;
	int startTimeN;

        	
	threadID = ((struct myargs *)inputStruct)->threadID;
        startTimeN = ((struct myargs *)inputStruct)->sleepingTime; 

        printf("HHHHHH %8d HHHHHH    A hunter is travelling to the valley\n", threadID);

         /* graze */
	if( startTimeN > 0) {
		usleep( startTimeN);
	}

        if( terminateFlag != 1) {
                printf("HHHHHH %8d HHHHHH    hunter arrives after travelling for %lf ms\n", threadID, startTimeN/1000.0);
                printf("HHHHHH %8d HHHHHH    hunter on the path wakes Smaug if he is sleeping\n", threadID );
        }
 
	pthread_mutex_lockChecked(&protectHuntersWaiting);
        huntersWaitingCounter = thievesWaitingCounter+1;
	pthread_mutex_unlockChecked(&protectHuntersWaiting);
 
	sem_postChecked(&dragonSleeping);
	sem_waitChecked(&huntersWaiting);
        printf("HHHHHH %8d HHHHHH    hunter is fighting with Smaug\n", threadID);
	sem_postChecked(&dragonFighting);
        printf("HHHHHH %8d HHHHHH    The hunter has completed a battle \n", threadID);
	sem_waitChecked(&huntersDefeated);
        printf("HHHHHH %8d HHHHHH    A hunter leaves \n", threadID);
	pthread_exit(NULL);
 }


void *cow(void *inputStruct)
{
	int threadId;
	int startTimeN;
	int k;

        	
	threadId = ((struct myargs *)inputStruct)->threadID;
        startTimeN = ((struct myargs *)inputStruct)->sleepingTime; 

	/* graze */
	printf("CCCCCC %8d CCCCCC    cow is born\n", threadId);
	if( startTimeN > 0) {
		usleep( startTimeN);
	}
	printf("CCCCCC %8d CCCCCC    cow grazes for %f ms\n", threadId, startTimeN/1000.0);


	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	pthread_mutex_lockChecked(&protectSheepInGroup);
	pthread_mutex_lockChecked(&protectCowsInGroup);
	sem_postChecked(&cowsInGroup);
	cowsInGroupCounter += 1;
	printf("CCCCCC %8d CCCCCC    %-2d cows have been enchanted \n", threadId, cowsInGroupCounter );
	if( (sheepInGroupCounter >= SHEEP_IN_GROUP )&&
			( cowsInGroupCounter  >= COWS_IN_GROUP )) {
		cowsInGroupCounter -= COWS_IN_GROUP;
		sem_waitChecked(&cowsInGroup);
		printf("CCCCCC %8d CCCCCC    The last cow is waiting\n", threadId);
		pthread_mutex_unlockChecked(&protectCowsInGroup);
		sheepInGroupCounter -= SHEEP_IN_GROUP;
		for (k=0; k<SHEEP_IN_GROUP; k++){
			sem_waitChecked(&sheepInGroup);
		}
		pthread_mutex_unlockChecked(&protectSheepInGroup);
		printf("CCCCCC %8d CCCCCC    The last sheep is waiting\n", threadId);
		pthread_mutex_lockChecked(&protectMealWaitingFlag);
		mealWaitingFlag += 1;
		printf("CCCCCC %8d CCCCCC    signal meal flag %d\n", threadId, mealWaitingFlag);
		pthread_mutex_unlockChecked(&protectMealWaitingFlag);
		sem_postChecked(&dragonSleeping);
		printf("CCCCCC %8d CCCCCC    last cow  wakes the dragon \n", threadId);
	}
	else
	{
		pthread_mutex_unlockChecked(&protectCowsInGroup);
		pthread_mutex_unlockChecked(&protectSheepInGroup);
	}

	sem_waitChecked(&cowsWaiting);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	pthread_mutex_lockChecked(&protectSheepEaten);
	pthread_mutex_lockChecked(&protectCowsEaten);
	sem_postChecked(&cowsEaten);
	cowsEatenCounter += 1;
	if( ( sheepEatenCounter >= SHEEP_IN_GROUP )&&
			( cowsEatenCounter >= COWS_IN_GROUP )) {
		cowsEatenCounter -= 1;
		sem_waitChecked(&cowsEaten);
		printf("CCCCCC %8d CCCCCCC     A cow has been eaten\n", threadId);
		pthread_mutex_unlockChecked(&protectCowsEaten);
		for (k=0; k<SHEEP_IN_GROUP; k++){
			sem_waitChecked(&sheepEaten);
		}
		sheepEatenCounter -= SHEEP_IN_GROUP;
		printf("CCCCCC %8d CCCCCCC     3 sheep have been eaten\n", threadId);
		pthread_mutex_unlockChecked(&protectSheepEaten);
		sem_postChecked(&dragonEating);
	}
	else
	{
		pthread_mutex_unlockChecked(&protectCowsEaten);
		pthread_mutex_unlockChecked(&protectSheepEaten);
		printf("CCCCCC %8d CCCCCC    A cow is waiting to be eaten\n", threadId);
	}
	sem_waitChecked(&cowsDead);

	printf("CCCCCC %8d CCCCCC    cow  dies\n", threadId);
	pthread_exit(NULL);
}

void terminateSimulation() {
	/* Init to zero, no elements are produced yet */
	sem_destroyChecked(&sheepInGroup);
	sem_destroyChecked(&cowsInGroup);
	sem_destroyChecked(&sheepWaiting);
	sem_destroyChecked(&cowsWaiting);
	sem_destroyChecked(&huntersWaiting);
	sem_destroyChecked(&thievesWaiting);
	sem_destroyChecked(&sheepEaten);
	sem_destroyChecked(&cowsEaten);
	sem_destroyChecked(&sheepDead);
	sem_destroyChecked(&cowsDead);
	sem_destroyChecked(&huntersDefeated);
	sem_destroyChecked(&thievesDefeated);
	sem_destroyChecked(&dragonSleeping);
	sem_destroyChecked(&dragonEating);
	sem_destroyChecked(&dragonPlaying);
	sem_destroyChecked(&dragonFighting);
	printf("XXXXTERMINATETERMINATE   semaphores destroyed\n");
	
	/* Init Mutex to one */
	pthread_mutex_destroyChecked(&protectSheepInGroup);
	pthread_mutex_destroyChecked(&protectCowsInGroup);
	pthread_mutex_destroyChecked(&protectMealWaitingFlag);
	pthread_mutex_destroyChecked(&protectHuntersWaiting);
	pthread_mutex_destroyChecked(&protectThievesWaiting);
	pthread_mutex_destroyChecked(&protectSheepEaten);
	pthread_mutex_destroyChecked(&protectCowsEaten);
	printf("XXXXTERMINATETERMINATE   mutexes destroyed\n");

	printf("GOODBYE from terminate \n");
}

int sem_waitChecked(sem_t *semaphoreID)
{
	int returnValue;
        returnValue = sem_wait(semaphoreID);
        if (returnValue == -1 ) {
            printf("semaphore wait failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
 	}
        return returnValue;
}

int sem_postChecked(sem_t *semaphoreID)
{
	int returnValue;
        returnValue = sem_post(semaphoreID);
        if (returnValue < 0 ) {
            printf("semaphore post operation failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value)
{
        int returnValue;
        returnValue = sem_init(semaphoreID, pshared, value);
        if (returnValue < 0 ) {
            printf("semaphore init operation failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int sem_destroyChecked(sem_t *semaphoreID)
{
        int returnValue;
        returnValue = sem_destroy(semaphoreID);
        if (returnValue < 0 ) {
            printf("semaphore destroy operation failed: terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int pthread_mutex_lockChecked(pthread_mutex_t *mutexID)
{
        int returnValue;
        returnValue = pthread_mutex_lock(mutexID);
        if (returnValue < 0 ) {
            printf("pthread mutex lock operation failed: terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID)
{
        int returnValue;
        returnValue = pthread_mutex_unlock(mutexID);
        if (returnValue < 0 ) {
            printf("pthread mutex unlock operation failed: terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int pthread_mutex_initChecked(pthread_mutex_t *mutexID,
	                              const pthread_mutexattr_t *attrib)
{
	int returnValue;
        returnValue = pthread_mutex_init(mutexID, attrib);
        if (returnValue < 0 ) {
            printf("pthread init operation failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}

int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID)
{
        int returnValue;
        returnValue = pthread_mutex_destroy(mutexID);
        if (returnValue < 0 ) {
            printf("pthread destroy failed: simulation terminating\n");
	    terminateSimulation();
            exit(0);
        }
        return returnValue;
}


