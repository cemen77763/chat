#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>

struct msgbuf_chat{
	long mtype;
	long num;
	char user_name[30];
	char message[255];
};

struct msgbuf_user{
	long mtype;
	char user_name[30];	
	long num;
};

long USERS_NUM = 0;

void *givenum(void *thread_data);

void *usersnum(void *thread_data);

void *usersmes(void *thread_data);

void main(){
	key_t key1, key2;
	int chat_id, users_id;
	int working = 1;
	char command[30];

	struct msgbuf_chat *chat;
	struct msgbuf_user user;

	pthread_t tid[3];

	key1 = 1234;
	key2 = 1233;
	if ((key1 == -1) || (key2 == -1)) exit(EXIT_FAILURE);

	users_id = msgget(key1, IPC_CREAT | 00666);
	chat_id = msgget(key2, IPC_CREAT | 00666);
	if ((users_id == -1) || (chat_id == -1)) exit(EXIT_FAILURE);

	pthread_create(&(tid[0]), NULL, usersnum, &(users_id));
	pthread_create(&(tid[1]), NULL, usersmes, &(chat_id));
	pthread_create(&(tid[2]), NULL, givenum, &(chat_id));


	while(working){
		printf("Write command: ");
		scanf("%s", command);
		if (command[0] == 'q') break;
	}
	
	msgctl(chat_id, IPC_RMID, NULL);
	msgctl(users_id, IPC_RMID, NULL);
	pthread_cancel(tid[0]);
	pthread_cancel(tid[1]);
	pthread_cancel(tid[2]);
	exit(EXIT_SUCCESS);
}
void *givenum(void *thread_data){
	int working = 1;
	long num = 4;
	int *id = (int*)thread_data;
	struct msgbuf_chat chat;

	while(working){
		msgrcv(*id, &(chat), sizeof(struct msgbuf_chat), 1, 0);
		chat.num = num;
		chat.mtype = 2;
		msgsnd(*id, &(chat), sizeof(struct msgbuf_chat), 0);
		num++;
	}
	pthread_exit(EXIT_SUCCESS);
}

void *usersmes(void *thread_data){
	int working = 1, i, k = 4;
	int *id = (int*)thread_data;
	struct msgbuf_chat chat;

	while(working){
		msgrcv(*id, &(chat), sizeof(struct msgbuf_chat), 3, 0);

		for (i = 4; i < USERS_NUM + 4; i++){
			chat.mtype = i;
			msgsnd(*id, &(chat), sizeof(struct msgbuf_chat), 0);
		}
	}

	pthread_exit(EXIT_SUCCESS);
}

void *usersnum(void *thread_data){
	int working = 1, i, j = 2;
	int *id = (int*)thread_data;
	struct msgbuf_user *users;
	users = malloc(sizeof(struct msgbuf_user));

	while(working){
		users = realloc(users, sizeof(struct msgbuf_user)*(USERS_NUM + 1));
		msgrcv(*id, &(users[USERS_NUM]), sizeof(struct msgbuf_user), 1, 0);
		USERS_NUM++;
		users[USERS_NUM - 1].num = USERS_NUM;
		users[USERS_NUM - 1].mtype = 2;
		msgsnd(*id, &(users[USERS_NUM - 1]), sizeof(struct msgbuf_user), 0);

		for (i = 0; i < USERS_NUM; i++){
			for (j = 0; j < USERS_NUM; j++){
				users[j].mtype = i + 3;
			}
			for (j = 0; j < USERS_NUM; j++){
				msgsnd(*id, &(users[j]), sizeof(struct msgbuf_user), 0);
			}
			users[i].num = 665;
			msgsnd(*id, &(users[i]), sizeof(struct msgbuf_user), 0);
			users[i].num = i + 1;	
		}
	}

	free(users);
	pthread_exit(EXIT_SUCCESS);
}