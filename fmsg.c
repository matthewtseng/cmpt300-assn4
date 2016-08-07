#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define min_v_interval 100
#define max_loads 11
#define v_wait 11
#define v_late 12
#define v_switch 13
#define v_maxspace 6
#define v_maxtrucks 2
#define v_car 1
#define v_truck 2
#define s_car 1
#define s_truck 2
#define loaded 3
#define enroute 4
#define arrived 5
#define unload 9
#define unload_ACK 10


typedef struct mymsgbuf
{
	long mtype;
	int data;
	int pid;
} mess_t;

mess_t msg;
int msgSize;
int queue_c2v;
int queue_v2c;
int queue_wait;
int queue_late;
int queue_boarding;
int queue_onferry;

int main_PID;
int capt_PID;
int v_maker_PID;
int v_PID;

int truck_chance;
int max_interval;

int timeChange( const struct timeval startTime );
void init();
void cleanup();

int car() {
	int localpid = getpid();
	setpgid(localpid, v_PID);
	printf("VEHICLE CAR     Car %d arrives at ferry dock\n", localpid);

	while(1) {
		msg.mtype = v_car; msg.pid = localpid;

		if(msgsnd(queue_v2c, &msg, msgSize, 0) == -1) { return -1; }


		if(msgrcv(queue_c2v, &msg, msgSize, localpid, 0) == -1) { return -1; }
		msg.mtype = v_car; msg.pid = localpid;
		if(msg.data == v_late) {
			printf("VEHICLE CAR     Car %d ACKs %s\n", localpid,  "late");
			if(msgsnd(queue_late, &msg, msgSize, 0) == -1) { return -1; }
		} else {
			printf("VEHICLE CAR     Car %d ACKs %s\n", localpid,  "waiting");
			if(msgsnd(queue_wait, &msg, msgSize, 0) == -1) { return -1; }
		}


		if(msgrcv(queue_boarding, &msg, msgSize, localpid, 0) == -1) { return -1; }

		if(msg.data != v_switch)
			break;
		else
			printf("VEHICLE CAR     Car %d switches from late to wait line\n", localpid);
	}
	printf("VEHICLE CAR     Car %d ACKs Captain's signal to leave queue\n", localpid);

	printf("VEHICLE CAR     Car %d is onboard the ferry\n", localpid);
	msg.mtype = loaded; msg.data = v_car; msg.pid = localpid;
	if(msgsnd(queue_onferry, &msg, msgSize, 0) == -1) { return -1; }


	if(msgrcv(queue_onferry, &msg, msgSize, enroute, 0) == -1) { return -1; }
	printf("VEHICLE CAR     Car %d ACKs Captain's sailing signal\n", localpid);


	if(msgrcv(queue_onferry, &msg, msgSize, arrived, 0) == -1) { return -1; }
	printf("VEHICLE CAR     Car %d ACKs Captain's arrival signal\n", localpid);


	if(msgrcv(queue_onferry, &msg, msgSize, unload, 0) == -1) { return -1; }
	printf("VEHICLE CAR     Car %d ACKs Captain's unload signal and unloads off ferry\n", localpid);


	printf("VEHICLE CAR     Car %d tells Captain that it has unloaded\n", localpid);
	msg.mtype = unload_ACK; msg.data = v_car; msg.pid = localpid;
	if(msgsnd(queue_onferry, &msg, msgSize, 0) == -1) { return -1; }

	return 0;
}

int truck() {
	int localpid = getpid();
	setpgid(localpid, v_PID);
	printf("VEHICLE TRUCK   Truck %d arrives at ferry dock\n", localpid);

	while(1) {
		msg.mtype = v_truck; msg.pid = localpid;
		if(msgsnd(queue_v2c, &msg, msgSize, 0) == -1) { return -1; }

		if(msgrcv(queue_c2v, &msg, msgSize, localpid, 0) == -1) { return -1; }
		msg.mtype = v_truck; msg.pid = localpid;
		if(msg.data == v_late) {
			printf("VEHICLE TRUCK   Truck %d ACKs %s\n", localpid,  "late");
			if(msgsnd(queue_late, &msg, msgSize, 0) == -1) { return -1; }
		} else {
			printf("VEHICLE TRUCK   Truck %d ACKs %s\n", localpid,  "waiting");
			if(msgsnd(queue_wait, &msg, msgSize, 0) == -1) { return -1; }
		}

		if(msgrcv(queue_boarding, &msg, msgSize, localpid, 0) == -1) { return -1; }
		if(msg.data != v_switch)
			break;
		else
		printf("VEHICLE TRUCK   Truck %d switches from late to wait line\n", localpid);
	}
	printf("VEHICLE TRUCK   Truck %d ACKs Captain's signal to leave queue\n", localpid);

	printf("VEHICLE TRUCK   Truck %d is onboard the ferry\n", localpid);
	msg.mtype = loaded; msg.data = v_truck; msg.pid = localpid;
	if(msgsnd(queue_onferry, &msg, msgSize, 0) == -1) { return -1; }

	if(msgrcv(queue_onferry, &msg, msgSize, enroute, 0) == -1) { return -1; }
	printf("VEHICLE TRUCK   Truck %d ACKs Captain's sailing signal\n", localpid);

	if(msgrcv(queue_onferry, &msg, msgSize, arrived, 0) == -1) { return -1; }
	printf("VEHICLE TRUCK   Truck %d ACKs Captain's arrival signal\n", localpid);

	if(msgrcv(queue_onferry, &msg, msgSize, unload, 0) == -1) { return -1; }
	printf("VEHICLE TRUCK   Truck %d ACKs Captain's unload signal and unloads off ferry\n", localpid);

	printf("VEHICLE TRUCK   Truck %d tells Captain that it has unloaded\n", localpid);
	msg.mtype = unload_ACK; msg.data = v_truck; msg.pid = localpid;
	if(msgsnd(queue_onferry, &msg, msgSize, 0) == -1) { return -1; }

	return 0;
}

int captain() {
	char *vehicleType = "";
	int currentLoad = 1;
	int numberOfCarsLoaded = 0;
	int numberOfTrucksLoaded = 0;
	int numberOfSpacesFilled = 0;
	int counter = 0;

	printf("CAPTAIN HOOK    Captain Process created. PID is: %d\n", getpid());

	while(currentLoad < max_loads) {
		numberOfTrucksLoaded = 0;
		numberOfCarsLoaded = 0;
		numberOfSpacesFilled = 0;

		printf("CAPTAIN HOOK    Load %d/%d started\n", currentLoad, max_loads);
		printf("CAPTAIN HOOK    The ferry is about to load!\n");


		while(msgrcv(queue_v2c, &msg, msgSize, 0, IPC_NOWAIT) != -1) {
			vehicleType = msg.mtype == v_car ? "Car" : "Truck";
			printf("CAPTAIN HOOK    Captain tells %s %d that it is waiting\n", vehicleType, msg.pid);
			msg.mtype = msg.pid; msg.data = v_wait;
			if(msgsnd(queue_c2v, &msg, msgSize, 0) == -1) {
				return -1;
			}
		}

		while(numberOfSpacesFilled < v_maxspace) {

			while(msgrcv(queue_v2c, &msg, msgSize, 0, IPC_NOWAIT) != -1) {
				vehicleType = msg.mtype == v_car ? "Car" : "Truck";
				printf("CAPTAIN HOOK    Captain tells %s %d that it is late\n", vehicleType, msg.pid);
				msg.mtype = msg.pid; msg.data = v_late;
				if(msgsnd(queue_c2v, &msg, msgSize, 0) == -1) {
					return -1;
				}
			}


			while(numberOfTrucksLoaded < v_maxtrucks &&
					numberOfSpacesFilled + s_truck <= v_maxspace &&
					msgrcv(queue_wait, &msg, msgSize, v_truck, IPC_NOWAIT) != -1) {
				printf("CAPTAIN HOOK    Captain tells Truck %d to leave waiting queue\n", msg.pid);
				msg.mtype = msg.pid;
				if(msgsnd(queue_boarding, &msg, msgSize, 0) == -1) {
					return -1;
				}
				numberOfTrucksLoaded++;
				numberOfSpacesFilled += s_truck;
			}


			while(numberOfSpacesFilled + s_car <= v_maxspace &&
					msgrcv(queue_wait, &msg, msgSize, v_car, IPC_NOWAIT) != -1) {
				printf("CAPTAIN HOOK    Captain tells Car %d to leave waiting queue\n", msg.pid);
				msg.mtype = msg.pid;
				if(msgsnd(queue_boarding, &msg, msgSize, 0) == -1) {
					return -1;
				}
				numberOfCarsLoaded++;
				numberOfSpacesFilled += s_car;
			}


			while(numberOfTrucksLoaded < v_maxtrucks &&
					numberOfSpacesFilled + s_truck <= v_maxspace &&
					msgrcv(queue_late, &msg, msgSize, v_truck, IPC_NOWAIT) != -1) {
				printf("CAPTAIN HOOK    Captain tells Truck %d to leave late queue\n", msg.pid);
				msg.mtype = msg.pid;
				if(msgsnd(queue_boarding, &msg, msgSize, 0) == -1) {
					return -1;
				}
				numberOfTrucksLoaded++;
				numberOfSpacesFilled += s_truck;
			}
			while(numberOfSpacesFilled + s_car <= v_maxspace &&
					msgrcv(queue_late, &msg, msgSize, v_car, IPC_NOWAIT) != -1) {
				printf("CAPTAIN HOOK    Captain tells Car %d to leave late queue\n", msg.pid);
				msg.mtype = msg.pid;
				if(msgsnd(queue_boarding, &msg, msgSize, 0) == -1) {
					return -1;
				}
				numberOfCarsLoaded++;
				numberOfSpacesFilled += s_car;
			}


			while(1) {
				int ret = msgrcv(queue_onferry, &msg, msgSize, loaded, IPC_NOWAIT);
				if(ret != -1) {
					vehicleType = msg.data == v_car ? "Car" : "Truck";
					printf("CAPTAIN HOOK    Captain ACKs that %s %d is onboard\n", vehicleType, msg.pid);

				} else if (errno == ENOMSG) {
					break;

				} else { return -1; }
			}


			if(numberOfCarsLoaded * s_car + numberOfTrucksLoaded * s_truck  == v_maxspace) {
				printf("CAPTAIN HOOK    The ferry is full!\n");
				printf("CAPTAIN HOOK    Captain signals ferry that is enroute\n");
				msg.mtype = enroute;
				for(counter = 0; counter < numberOfCarsLoaded + numberOfTrucksLoaded; counter++)
					if(msgsnd(queue_onferry, &msg, msgSize, 0) == -1) { return -1; }
				printf("CAPTAIN HOOK    All vehicles ack'd they are sailing\n");
				printf("CAPTAIN HOOK    Arrived at destination and docked\n");
				printf("CAPTAIN HOOK    Captain tells the boarded vehicles that they have arrived\n");
				msg.mtype = arrived;
				for(counter = 0; counter < numberOfCarsLoaded + numberOfTrucksLoaded; counter++)
					if(msgsnd(queue_onferry, &msg, msgSize, 0) == -1) { return -1; }

				printf("CAPTAIN HOOK    Captain tells the boarded vehicles to unload\n");
				msg.mtype = unload;
				for(counter = 0; counter < numberOfCarsLoaded + numberOfTrucksLoaded; counter++)
					if(msgsnd(queue_onferry, &msg, msgSize, 0) == -1) { return -1; }



				counter = 0;
				while(counter < numberOfCarsLoaded + numberOfTrucksLoaded) {
					int ret = msgrcv(queue_onferry, &msg, msgSize, unload_ACK, IPC_NOWAIT);
					if(ret == msgSize) {
						vehicleType = msg.data == v_car ? "Car" : "Truck";
						printf("CAPTAIN HOOK    Captain ACKs that %s %d has unloaded\n", vehicleType, msg.pid);
						counter++;

					} else if(ret == -1 && errno != ENOMSG) { return -1; }
				}
				printf("CAPTAIN HOOK    All vehicles have unloaded\n");

				while(msgrcv(queue_late, &msg, msgSize, 0, IPC_NOWAIT) != -1) {
					msg.mtype = msg.pid; msg.data = v_switch;
					if(msgsnd(queue_boarding, &msg, msgSize, 0) == -1) { return -1; }
				}
				printf("CAPTAIN HOOK    All late vehicles now waiting\n");


			}

		}

		printf("\n\n\n");
		printf("CAPTAIN HOOK    Arrived at main loading dock from destination\n");
		currentLoad++;
		if(currentLoad >= max_loads)
			printf("CAPTAIN HOOK    Load %d/%d started. Done.\n", currentLoad, max_loads);
	}

	return 0;
}

int vehiclemaker() {
	int localpid = getpid();
	struct timeval startTime;
	int elapsed = 0;
	int lastArrivalTime = 0;

	printf("PROCESS MAKER   Vehicle Maker Process created. PID is: %d \n", localpid);
	gettimeofday(&startTime, NULL);
	elapsed = timeChange(startTime);
	srand (elapsed*1000+44);
	while(1) {

		elapsed = timeChange(startTime);
		if(elapsed >= lastArrivalTime) {
			printf("PROCESS MAKER   Elapsed time %d, Arrival time %d\n", elapsed, lastArrivalTime);
			if(lastArrivalTime > 0 ) {
				if(rand() % 100 < truck_chance ) {
					printf("PROCESS MAKER   Created a truck process\n");
					int childPID = fork();
					if(childPID == 0) {
						return truck();
					} else if(childPID == -1) {
						return -1;
					}
				}
				else {
					printf("PROCESS MAKER   Created a car process\n");
					int childPID = fork();
					if(childPID == 0) {
						return car();
					} else if(childPID == -1) {
						return -1;
					}
				}
			}
			lastArrivalTime += rand()% max_interval;
			printf("PROCESS MAKER   present time %d, next arrival time %d\n", elapsed, lastArrivalTime);
		}
	}
	printf("PROCESS MAKER   Vehicle Maker Process ended\n");
	return 0;
}

int main() {
	int status;
	init();

	printf("Please enter integer values for the following variables\n");


	printf("Enter the percent probability that the next vehicle is a truck (0..100): ");
	scanf("%d", &truck_chance);
	while(truck_chance < 0 || truck_chance > 100) {
		printf("Probability must be between 0 and 100: ");
		scanf("%d", &truck_chance);
	}


	printf("Enter the maximum length of the interval between vehicles (%d..MAX_INT): ", min_v_interval);
	scanf("%d", &max_interval);
	while(max_interval < min_v_interval) {
		printf("Interval must be greater than %d: ", min_v_interval);
		scanf("%d", &max_interval);
	}

	if((capt_PID = fork()) == 0) {
		return captain();
	}

	if((v_maker_PID = fork()) == 0) {
		return vehiclemaker();
	}

	waitpid(capt_PID, &status, 0);
	kill(v_maker_PID, SIGKILL);
	waitpid(v_maker_PID, &status, 0);

	msgctl(queue_c2v , IPC_RMID , 0);
	msgctl(queue_v2c , IPC_RMID , 0);
	msgctl(queue_wait , IPC_RMID , 0);
	msgctl(queue_late , IPC_RMID , 0);
	msgctl(queue_boarding , IPC_RMID , 0);
	msgctl(queue_onferry , IPC_RMID , 0);
	return 0;
}

void init() {

	msgSize = sizeof(mess_t) - sizeof(long);
	if((queue_c2v = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("STARTING UP..   Failed to create queue queue_c2v\n");
		exit(1);
	}
	if((queue_v2c = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("STARTING UP..   Failed to create queue queue_v2c\n");
		exit(1);
	}
	if((queue_wait = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("STARTING UP..   Failed to create queue queue_wait\n");
		exit(1);
	}
	if((queue_late = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("STARTING UP..   Failed to create queue queue_late\n");
		exit(1);
	}
	if((queue_boarding = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("STARTING UP..   Failed to create queue queue_boarding\n");
		exit(1);
	}
	if((queue_onferry = msgget(IPC_PRIVATE, IPC_CREAT | 0660)) == -1) {
		printf("STARTING UP..   Failed to create queue queue_onferry\n");
		exit(1);
	}
	printf("STARTING UP..   All queues successfully created\n");
	main_PID = getpid();
	v_PID = main_PID - 1;
}

int timeChange( const struct timeval startTime ) {
	struct timeval nowTime;
	long int elapsed;
	int elapsedTime;

	gettimeofday(&nowTime, NULL);
	elapsed = (nowTime.tv_sec - startTime.tv_sec) * 1000000 + (nowTime.tv_usec - startTime.tv_usec);
	elapsedTime = elapsed / 1000;
	return elapsedTime;
}
