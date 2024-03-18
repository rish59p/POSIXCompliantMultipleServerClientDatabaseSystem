// secondary.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <string.h>

struct Message {
    long int mType;
    int sqno;
    int opno;
    char filename[100];
};

struct Result {
    long mtype;
    char message[100];
    char output[100];
};

struct SharedMemory {
    int numberOfNodes;
    int adjacencyMatrix[30][30]; // Assuming a maximum of 30 nodes
};

struct ThreadArg {
    int vertex;
    int msqid;
    int sqno;
    int opno;
};

sem_t dfsBfsSemaphore;
struct SharedMemory *sharedMemory;
int shmid;

void *dfsThread(void *arg) {
    struct ThreadArg *currentThread = (struct ThreadArg *)arg;
    int node = currentThread->vertex;

    int visited[30] = {0};
    char result[100] = "";

    for (int i = 0; i < sharedMemory->numberOfNodes; i++) {
        if (sharedMemory->adjacencyMatrix[node][i] && !visited[i]) {
            dfs(i, visited, result);
        }
    }

    // Send the DFS result back to the client
    struct Result dfsResult;
    dfsResult.mtype = 8 * (currentThread->sqno) + currentThread->opno;
    strcpy(dfsResult.output, result);

    if (msgsnd(currentThread->msqid, &dfsResult, sizeof(struct Result), 0) == -1) {
        perror("Error sending message to the Client");
        exit(EXIT_FAILURE);
    }

    pthread_exit(NULL);
}

void dfs(int node, int visited[], char result[]) {
    visited[node] = 1;

    pthread_t threads[30];
    int threadCount = 0;

    struct ThreadArg threadArgs[30];

    for (int i = 0; i < sharedMemory->numberOfNodes; i++) {
        if (sharedMemory->adjacencyMatrix[node][i] && !visited[i]) {
            // Create a thread for each unvisited node
            threadArgs[threadCount].vertex = i;
            threadArgs[threadCount].msqid = currentThread->msqid;
            threadArgs[threadCount].sqno = currentThread->sqno;
            threadArgs[threadCount].opno = currentThread->opno;

            pthread_create(&threads[threadCount], NULL, dfsThread, (void *)&threadArgs[threadCount]);
            threadCount++;
        }
    }

    // Wait for all child threads to terminate
    for (int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }

    strcat(result, " ");
    strcat(result, currentThread->vertex); // Assuming result is a space-separated list of vertices
}

void *bfsThread(void *arg) {
    struct ThreadArg *currentThread = (struct ThreadArg *)arg;
    int startingVertex = currentThread->vertex;

    int visited[30] = {0};
    char result[100] = "";

    visited[startingVertex] = 1;

    pthread_t threads[30];
    int threadCount = 0;

    struct ThreadArg threadArgs[30];

    for (int i = 0; i < sharedMemory->numberOfNodes; i++) {
        if (sharedMemory->adjacencyMatrix[startingVertex][i] && !visited[i]) {
            // Create a thread for each unvisited node
            threadArgs[threadCount].vertex = i;
            threadArgs[threadCount].msqid = currentThread->msqid;
            threadArgs[threadCount].sqno = currentThread->sqno;
            threadArgs[threadCount].opno = currentThread->opno;

            pthread_create(&threads[threadCount], NULL, bfsThread, (void *)&threadArgs[threadCount]);
            threadCount++;
        }
    }

    // Wait for all child threads to terminate
    for (int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }

    // Send the BFS result back to the client
    struct Result bfsResult;
    bfsResult.mtype = 8 * (currentThread->sqno) + currentThread->opno;
    strcpy(bfsResult.output, result);

    if (msgsnd(currentThread->msqid, &bfsResult, sizeof(struct Result), 0) == -1) {
        perror("Error sending message to the Client");
        exit(EXIT_FAILURE);
    }

    pthread_exit(NULL);
}

void bfs(int startingVertex) {
    struct ThreadArg bfsArgs;
    bfsArgs.vertex = startingVertex;
    bfsArgs.msqid = currentThread->msqid;
    bfsArgs.sqno = currentThread->sqno;
    bfsArgs.opno = currentThread->opno;

    bfsThread((void *)&bfsArgs);
}

void *handleRead(void *arg) {
    struct ThreadArg *currentThread = (struct ThreadArg *)arg;

    key_t shmKey = ftok("/mySharedMemory", 'B' + currentThread->sqno);
    shmid = shmget(shmKey, sizeof(struct SharedMemory), 0666);
    if (shmid == -1) {
        perror("Error accessing shared memory segment");
        exit(EXIT_FAILURE);
    }

    // Read starting vertex from shared memory
    sharedMemory = (struct SharedMemory *)shmat(shmid, NULL, 0);
    int startingVertex = sharedMemory->numberOfNodes;

    if (currentThread->opno == 3) {
        sem_wait(&dfsBfsSemaphore); // Wait for semaphore
        char dfsResult[100] = "";
        dfs(startingVertex, (int[30]){0}, dfsResult);
        sem_post(&dfsBfsSemaphore); // Release semaphore
    } else if (currentThread->opno == 4) {
        bfs(startingVertex);
    }

    // Detach shared memory segment
    if (shmdt(sharedMemory) == -1) {
        perror("Error detaching shared memory segment");
        exit(EXIT_FAILURE);
    }

    pthread_exit(NULL);
}

int main() {
    sem_init(&dfsBfsSemaphore, 0, 1);

    key_t msgQueueKey = ftok("/myMsgQueue", 'A'); // Unique key for the message queue
    int msqid = msgget(msgQueueKey, 0666);
    if (msqid == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }

    while (1) {
        struct Message msg;
        msgrcv(msqid, &msg, sizeof(msg), 3, 0);

        struct ThreadArg *arg = malloc(sizeof(struct ThreadArg));
        arg->vertex = msg.sqno;
        arg->msqid = msqid;
        arg->sqno = msg.sqno;
        arg->opno = msg.opno;

        pthread_t thread;
        pthread_create(&thread, NULL, handleRead, (void *)arg);
        pthread_detach(thread); // Detach the thread to avoid

    }

    // Clean up and terminate
    sem_destroy(&dfsBfsSemaphore);

    return 0;
}

