#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define pServerMtype 3
#define clientMtype 1

struct Message {
    long mType;
    int sqno;
    int opno;
    char filename[100]; // Adjust the size based on your needs
};

struct SharedMemory {
    int numberOfNodes;
    int adjacencyMatrix[30][30]; // Assuming a maximum of 30 nodes
};
struct Result{
	long mtype;
	char message[100];
	char output[100];
};

struct argument{
    struct Message message;
    int msqid;
};

void* handleWrite(void* arg);

int main() {

	key_t msgQueueKey = ftok("/tmp", 'A');
	int msqid = msgget(1234, 0666);
    printf("%d\n",msqid);
	
	while(1){
		struct Message msg;
		msgrcv(msqid, &msg, sizeof(msg),2,0);
        printf("%ld\n",msg.mType);
        struct argument arg;
        arg.message=msg;
        arg.msqid=msqid;
		pthread_t thread;
		pthread_create(&thread,NULL, handleWrite, (void*)&arg);
		pthread_join(thread,NULL);
	}
	return 0;
   	
}

void* handleWrite(void* arg){
  	struct argument Arg = *((struct argument *)arg);
    struct Message msg = Arg.message;
    
	key_t shmKey = ftok("/tmp", 'B' + msg.sqno); 
    int shmid = shmget(shmKey, sizeof(struct SharedMemory),0666|IPC_CREAT);
	
	// Attach shared memory segment to the process
    	struct SharedMemory *sharedMemory = (struct SharedMemory *)shmat(shmid, NULL, 0);
    	if (sharedMemory == (void *)-1) {
        	perror("Error attaching shared memory segment");
        	exit(EXIT_FAILURE);
   	}
   	
   	// Map the shared memory segment
//	struct SharedMemory SharedMemoryptr = mmap(NULL, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
//	if (SharedMemoryptr == MAP_FAILED) {
//        	perror("mmap");
//        	close(shmid);
//        	pthread_exit(NULL);
//  	}
  	

  	struct Result result;
    result.mtype= 8*(msg.sqno)+msg.opno;
  	
  	int option=msg.opno;
  	FILE* file;
  	if(option==1){
  		file=fopen(msg.filename,"w");
  		if (file == NULL) {
		    perror("Error opening file");
		    return 0;
		}
  		fprintf(file,"%d\n", sharedMemory->numberOfNodes);
  		for (int i = 0; i < sharedMemory->numberOfNodes; ++i) {
			for (int j = 0; j < sharedMemory->numberOfNodes; ++j) {
		        	fprintf(file, "%d ", sharedMemory->adjacencyMatrix[i][j]);
		    	}
		    	fprintf(file, "\n");
        	}
            strcpy(result.message,"File successfully added");
        	fclose(file);
  	}
  	else if (option==2){
        file = fopen(msg.filename, "r");
        if (file == NULL) {
            perror("Error opening file");
            return 0;
        }

        // Read the existing number of nodes
        if (fscanf(file, "%d", &(sharedMemory->numberOfNodes)) != 1) {
            perror("Error reading number of nodes");
            fclose(file);
            return 0;
        }

        // Read the existing adjacency matrix
        for (int i = 0; i < sharedMemory->numberOfNodes; ++i) {
            for (int j = 0; j < sharedMemory->numberOfNodes; ++j) {
                if (fscanf(file, "%d", &(sharedMemory->adjacencyMatrix[i][j])) != 1) {
                    perror("Error reading adjacency matrix");
                    fclose(file);
                    return 0;
                }
            }
        }

        fclose(file);

        file = fopen(msg.filename, "w");
        if (file == NULL) {
            perror("Error opening file");
            return 0;
        }

        fprintf(file, "%d\n", sharedMemory->numberOfNodes);
        for (int i = 0; i < sharedMemory->numberOfNodes; ++i) {
            for (int j = 0; j < sharedMemory->numberOfNodes; ++j) {
                fprintf(file, "%d ", sharedMemory->adjacencyMatrix[i][j]);
            }
            fprintf(file, "\n");
        }
        strcpy(result.message, "File successfully modified");
        fclose(file);
  	}
  	else{
  		printf("Invalid Request received\n");
  		
  	}		
//   	munmap(SharedMemoryptr, sizeof(struct SharedMemory));
        if (msgsnd(Arg.msqid, &result, sizeof(struct Result), 0) == -1) {
            perror("Error sending message to the Client");
            exit(EXIT_FAILURE);
        }
        close(shmid);

  	pthread_exit(NULL);
}
