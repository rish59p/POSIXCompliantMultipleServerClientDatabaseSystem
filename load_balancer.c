#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <fcntl.h>

struct Message {
    long mType;
    int sqno;
    int opno;
    char filename[100]; 
};


int main(){
	key_t msgQueueKey = ftok("/myMsgQueue", 'A'); // Unique key for the message queue
	int msqid = msgget(msgQueueKey, IPC_CREAT | 0666);   
	if (msqid == -1) {
		perror("Error creating message queue");
		exit(EXIT_FAILURE);
	}
	
	struct Message msg;
	printf("Load Balancer Started\n");
	while(1){
		if(msgrcv(msqid,&msg,sizeof(msg),1,0)==-1){		//1|4 will not work..nor will IPC_NOWAIT	
			perror("Error receiving from message queue");
			exit(EXIT_FAILURE);	
		}
		printf("Request recieved\n");

		if (msg.opno == 5) {
		    // Termination request from cleanup
		    // Check if the message queue is empty
		    //sending termination request to all the servers
			//sending to primary server
			struct Message msg1;
			msg1.mType = 2;
			msg1.opno = 5;
			if (msgsnd(msqid, &msg1, sizeof(struct Message), 0) == -1) {
				perror("Error sending message to the primary server");
				exit(EXIT_FAILURE);
			}
			//sending to secondary server
			struct Message msg2;
			msg2.mType = 3;
			msg2.opno = 5;
			if (msgsnd(msqid, &msg2, sizeof(struct Message), 0) == -1) {
				perror("Error sending message to the secondary server");
				exit(EXIT_FAILURE);
			}
		    struct msqid_ds msq_info;
		    while (1) {
		        if (msgctl(msqid, IPC_STAT, &msq_info) == -1) {
		            perror("Error getting message queue info");
		            exit(EXIT_FAILURE);
		        }

		        if (msq_info.msg_qnum == 0) {
		            // Message queue is empty, break the loop
		            break;
		        } else {
		            // Message queue is not empty, wait for a short time
		            sleep(1);
		        }
		    }

		    // Delete the message queue
		    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
		        perror("Error deleting message queue");
		        exit(EXIT_FAILURE);
		    } else {
		        printf("Message queue deleted successfully\n");
		        break; // Exit the main loop after deleting the message queue
            	}
        }
		else{
			if(msg.opno==1 || msg.opno==2){		//write
				msg.mType = 2;
				printf("Sending request to Primary serevr\n");
				if (msgsnd(msqid, &msg, sizeof(msg), 0) == -1) {
					perror("Error sending message to the primary server");
					exit(EXIT_FAILURE);
				}
			}
        
			else if(msg.opno==3 || msg.opno==4){	//read						
				msg.mType = 3;			

				if (msgsnd(msqid, &msg, sizeof(struct Message), 0) == -1) {
					perror("Error sending message to the secondary server");
					exit(EXIT_FAILURE);
				}
			}
			else{
				//continue
			}
		}
	}
	

	sleep(5);

	return 0;
}
