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


/* Constants */
#define MAX_SPOTS_ON_FERRY 6
#define MAX_LOADS 3

/* Threads to create and control vehicles */
pthread_t vehicleThread[200];
pthread_t captainThread;
pthread_t createVehicleThread;
int vehicleThreadCounter = 0;

/* Define semaphores and mutexes */
/* Mutexes for counter protection and the counters they protect */
pthread_mutex_t protectCarWaiting;
int carWaitingCounter = 0;
pthread_mutex_t protectTruckWaiting;
int truckWaitingCounter = 0;
pthread_mutex_t protectCarUnloaded;
int carUnloadedCounter = 0;
pthread_mutex_t protectTruckUnloaded;
int truckUnloadedCounter = 0;

/* Define semaphores to be placed in a single semaphore set */
/* Numbers indicate index in semaphore set for named semaphore */
sem_t carWaiting;
sem_t truckWaiting;
sem_t carLoaded;
sem_t truckLoaded;
sem_t vehiclesSailed;
sem_t vehiclesArrived;
sem_t carUnloaded;
sem_t truckUnloaded;
sem_t waitUnload;
sem_t readyUnload;
sem_t terminate;

/* Pointers and ids for functions */
void initialize();
void terminate();
void* car();
void* truck();
void* captain();
void* createVehicle();
int sem_waitChecked(sem_t *semaphoreID);
int sem_postChecked(sem_t *semaphoreID);
int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value);
int sem_destroyChecked(sem_t *semaphoreID);
int pthread_mutex_lockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_initChecked(pthread_mutex_t *mutexID, const pthread_mutexattr_t *attrib);
int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID);
int timeChange(const struct timeval startTime);

int truckArrivalProb; /* User supplied probability that the next vehicle is a truck */
int maxTimeToNextArrival; /* Maximum number of milliseconds between vehicle arrivals */

int main() 
{
	initialize();

	pthread_join(createVehicleThread, NULL);
	printf("Create vehicle joined\n");
	pthread_join(captainThread, NULL);
	printf("Captain joined\n");

	terminate();
	printf("Simulation has completed\n");
	exit(0);
}

void initialize()
{
	printf("Initialize start\n")

	/* Initialize to zero, no elements are produced yet */
	sem_initChecked(&carWaiting, 0, 0);
	sem_initChecked(&truckWaiting, 0, 0);
	sem_initChecked(&carLoaded, 0, 0);
	sem_initChecked(&truckLoaded, 0, 0);
	sem_initChecked(&vehiclesSailed, 0, 0);
	sem_initChecked(&vehiclesArrived, 0, 0);
	sem_initChecked(&carUnloaded, 0, 0);
	sem_initChecked(&truckUnloaded, 0, 0);
	sem_initChecked(&waitUnload, 0, 0);
	sem_initChecked(&readyUnload, 0, 0);
	sem_initChecked(&terminate, 0, 0);
	printf("Semaphores initialized\n");
	
	/* Initialize threads */
	pthread_create(&vehicleThread, NULL, createVehicle, NULL);
	pthread_create(&captainThread, NULL, captain, NULL);

	/* Initialize Mutex to one */
	pthread_mutex_initChecked(&protectCarWaiting, NULL);
	pthread_mutex_initChecked(&protectTruckWaiting, NULL);
	pthread_mutex_initChecked(&protectCarUnloaded, NULL);
	pthread_mutex_initChecked(&protectTruckUnloaded, NULL);
	printf("Mutexes initialized\n");

	printf("Initialize end\n");
}

void terminateSimulation()
{
	printf("Terminate start\n");

	pthread_mutex_destroy(&protectCarWaiting);
	pthread_mutex_destroy(&protectTruckWaiting);
	pthread_mutex_destroy(&protectCarUnloaded);
	pthread_mutex_destroy(&protectTruckUnloaded);
	printf("Mutexes destroyed\n");

	sem_destroyChecked(&carWaiting);
	sem_destroyChecked(&truckWaiting);
	sem_destroyChecked(&carLoaded);
	sem_destroyChecked(&truckLoaded);
	sem_destroyChecked(&vehiclesSailed);
	sem_destroyChecked(vehiclesArrived);
	sem_destroyChecked(carUnloaded);
	sem_destroyChecked(truckUnloaded);
	sem_destroyChecked(waitUnload);
	sem_destroyChecked(readyUnload);
	sem_destroyChecked(terminate);
	printf("Semaphores destroyed\n");

	printf("Terminate end\n");
}

void* createVehicle()
{
	/* Timing of car and truck creation */ 
	struct timeval startTime; /* Time at start of program execution */
	int elapsed = 0; /* time from start of program execution */
	int lastArrivalTime = 0; /* Time from start of program execution at which the last vehicle arrived */

	printf("Start of creating vehicles thread\n");

	/* Initialize start time */
	gettimeofday(&startTime, NULL);
	elapsed = timeChange(startTime);
	srand(time(0));

	while(1) 
	{
		elapsed = timeChange(startTime);
		while(elapsed >= lastArrivalTime)
		{
			printf("Creating vehicle! Elasped time: %d; Arrival time: %d\n", elapsed, lastArrivalTime);
			if (lastArrivalTime > 0) {
				if (rand() % 100 > truckArrivalProb)
				{
					/* Vehicle is a car */
					pthread_create(&(vehicleThread[vehicleThreadCounter]), NULL, car, NULL);
					printf("Created a car thread\n");
				}
				else
				{
					/* Else vehicle is a truck */
					pthread_create(&(vehicleThread[vehicleThreadCounter]), NULL, truck, NULL);
					printf("Created a truck thread\n");
				}
				vehicleThreadCounter++;
			}
			lastArrivalTime += rand() % maxTimeToNextArrival;
			printf(" Created vehicle! Elasped time: %d; Arrival Time: %d\n", elasped, lastArrivalTime);
		}
	}
	printf("End of creating vehicles thread");
}

void* captain()
{
	int loads = 0; /* Counter for how many loads have occurred */
	int numCarWaiting = 0; /* Used as placeholder for counter variable */
	int numTruckWaiting = 0; /* Used as placeholder for counter variable */
	int numSpacesFilled = 0; /* Used as counter for loading */
	int numSpacesEmpty /* Used as counter for unloading */
	int numVehicles = 0; /* Counter for total vehicles */
	int i = 0; /* Counter for the for loops used later on */

	printf("Captain process started\n");

	while (loads < LOAD_MAX) /* While load is less than 11 (LOAD_MAX) */
	{
		/* Since we prioritize trucks when loading ferry, we start off with trucks */
		/* Reset variables in every loop through the while */
		numTrucksLoaded = 0;
		numSpacesFilled = 0;
		numVehicles = 0;

		while (numSpacesFilled < MAX_SPOTS_ON_FERRY)
		{
			/* Grab counters values */
			pthread_mutex_lockChecked(&protectTruckWaiting);
			pthread_mutex_lockChecked(&protectCarWaiting);
			numTruckWaiting = truckWaitingCounter;
			numCarWaiting = carWaitingCounter;
			pthread_mutex_unlockChecked(&protectCarWaiting);
			pthread_mutex_unlockChecked(&protectTruckWaiting);

			/* Prioritizing trucks over cars, so check cars first */
			/* Check requirements for trucks and then add trucks when satisfied */
			while (numTruckWaiting > 0 && numSpacesFilled < MAX_SPOTS_ON_FERRY - 1 && numTrucksLoaded < 2) /* MAX_SPOTS_ON_FERRY - 1 (5); equivalent to having two trucks */
			{
				pthread_mutex_lockChecked(&protectTruckWaiting);
				truckWaitingCounter--;
				printf("Truck signalled to leave queue\n");
				sem_postChecked(&truckWaiting);
				pthread_mutex_unlockChecked(&protectTruckWaiting);
				numTrucksWaiting--;
				numTrucksLoaded++;
				numSpacesFilled = numSpacesFilled + 2;
				numVehicles++;
			}

			/* Check requirements for cars and then add cars when satisfied */
			while (numCarWaiting > 0 && numSpacesFilled < MAX_SPOTS_ON_FERRY)
			{
				pthread_mutex_lockChecked(&protectCarWaiting);
				carWaitingCounter--;
				printf("Car signalled to leave queue\n");
				sem_postChecked(&carWaiting);
				pthread_mutex_unlockChecked(&protectCarWaiting);
				numCarsWaiting--;
				numSpacesFilled++;
				numVehicles++;
			}
		}

		/* Loaded on to ferry */
		/* Trucks are loaded */
		for (i = 0; i < numTrucksLoaded; i++)
		{
			sem_waitChecked(&truckLoaded);
			printf("Truck loaded onto ferry successfully\n");
		}

		/* Cars are loaded */
		for (i = 0; i < numVehicles - numTrucksLoaded; i++)
		{
			sem_waitChecked(&carLoaded);
			printf("Car loaded onto ferry successfully\n");
		}

		/* After loading successfully, ferry waits for signals from vehicles */
		printf("Captain signals ferry is full and is now sailing\n")

		/* Waiting for vehicles to signal back to captain, after sets sail to destination */
		for (i = 0; i < numVehicles; i++) 
		{
			printf("Vehicle %d has arrived\n", i);
			sem_postChecked(&vehiclesArrived);
		}

		/* Signal vehicles to unload from ferry */
		for (i = 0; i < numVehicles; i++)
		{
			sem_postChecked(&waitUnload);
		}

		/* Unload trucks and cars */
		numSpacesEmpty = 0;
		while (numSpacesEmpty < MAX_SPOTS_ON_FERRY)
		{
			pthread_mutex_lockChecked(&protectTruckUnloaded);
			if (truckUnloadedCounter > 0)
			{
				sem_waitChecked(&truckUnloaded);
				truckUnloadedCounter--;
				numSpacesEmpty = numSpacesEmpty + 2;
				printf("A truck has unloaded from the ferry\n");
			}
			pthread_mutex_unlockChecked(&protectTruckUnloaded);

			pthread_mutex_lockChecked(&protectCarUnloaded);
			if (carUnloadedCounter > 0)
			{
				sem_waitChecked(&carUnloaded);
				carUnloadedCounter--;
				numSpacesEmpty++;
				printf("A car has unloaded from the ferry\n");
			}
			pthread_mutex_unlockChecked(&protectCarUnloaded);
		}
		printf("Vehicles have unloaded from the ferry successfully\n");

		/* Terminate vehicles that were unloaded at destination dock */
		for (i = 0; i < numVehicles; i++)
		{
			printf ("Terminating vehicle %d\n", i);
			sem_post(&terminate);
		}
		printf("\nCaptain has arrived at home dock\n");
		loads++;
	}
	exit(0);
}

void* truck()
{
	unsigned long int *threadID;

	/* Get the calling thread id; equivalent to getpid() for processes */
	threadID = (unsigned long int *)pthread_self(); 

	/* Car arrives at home dock */ 
	pthread_mutex_lockChecked(&protectTruckWaiting);
	truckWaitingCounter++;
	pthread_mutex_unlockChecked(&protectTruckWaiting);
	printf("Truck %5lu queued\n", *threadID);
	sem_waitChecked(&truckWaiting);
	printf("Truck %5lu leaving queue to load\n", *threadID);

	/* Captain signalled car to be loaded */
	printf("Truck %5lu onboard ferry\n", *threadID);
	sem_postChecked(&truckWaiting);

	/* Waiting for ferry to be full so it can sail */
	sem_waitChecked(&vehiclesSailed);
	printf("Truck %5lu travelling\n", *threadID);

	/* Sail across the river and wait for signal from captain */
	sem_waitChecked(&vehiclesArrived);
	prinf("Truck %5lu has arrived at destination dock\n", *threadID);
	sem_postChecked(&readyUnload);

	/* Unloading the ferry */
	sem_waitChecked(&waitUnload);
	pthread_mutex_lockChecked(&protectTruckUnloaded);
	truckUnloadedCounter++;
	pthread_mutex_unlockChecked(&protectTruckUnloaded);
	printf("Truck %5lu unloaded from ferry\n", *threadID);
	sem_postChecked(&truckUnloaded);

	/* Truck terminates itself */
	sem_waitChecked(&terminate);
	printf("Truck %5lu terminates\n", *threadID);
	pthread_exit(0);
}

void* car()
{
	unsigned long int *threadID;

	/* Get the calling thread id; equivalent to getpid() for processes */
	threadID = (unsigned long int *)pthread_self(); 

	/* Car arrives at home dock */ 
	pthread_mutex_lockChecked(&protectCarWaiting);
	carWaitingCounter++;
	pthread_mutex_unlockChecked(&protectCarWaiting);
	printf("Car %5lu queued\n", *threadID);
	sem_waitChecked(&carWaiting);
	printf("Car %5lu leaving queue to load\n", *threadID);

	/* Captain signalled car to be loaded */
	printf("Car %5lu onboard ferry\n", *threadID);
	sem_postChecked(&carWaiting);

	/* Waiting for ferry to be full so it can sail */
	sem_waitChecked(&vehiclesSailed);
	printf("Car %5lu travelling\n", *threadID);

	/* Sail across the river and wait for signal from captain */
	sem_waitChecked(&vehiclesArrived);
	prinf("Car %5lu has arrived at destination dock\n", *threadID);
	sem_postChecked(&readyUnload);

	/* Unloading the ferry */
	sem_waitChecked(&waitUnload);
	pthread_mutex_lockChecked(&protectCarUnloaded);
	carUnloadedCounter++;
	pthread_mutex_unlockChecked(&protectCarUnloaded);
	printf("Car %5lu unloaded from ferry\n", *threadID);
	sem_postChecked(&carUnloaded);

	/* Car terminates itself */
	sem_waitChecked(&terminate);
	printf("Car %5lu terminates\n", *threadID);
	pthread_exit(0);
}

int timeChange( const struct timeval startTime )
{
    struct timeval nowTime;
    long int elapsed;
    int elapsedTime;

    gettimeofday(&nowTime, NULL);
    elapsed = (nowTime.tv_sec - startTime.tv_sec) * 1000000
            + (nowTime.tv_usec - startTime.tv_usec);
    elapsedTime = elapsed / 1000;
    return elapsedTime;
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

int pthread_mutex_initChecked(pthread_mutex_t *mutexID, const pthread_mutexattr_t *attrib)
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