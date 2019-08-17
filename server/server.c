#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>

struct msgbuf_chat{
	long mtype;
	char user_name[30];
	char message[255];
};

struct msgbuf_user{
	long mtype;
	char user_name[30];	
	long num;
};

long USERS_NUM = 0;

void *usersnum(void *thread_data);

void main(){
	key_t key1, key2;
	int chat_id, users_id;
	int working = 1;
	char command[30];

	struct msgbuf_chat *chat;
	struct msgbuf_user user;

	pthread_t tid;

	key1 = 1234;
	key2 = ftok("./server", 2);
	if ((key1 == -1) || (key2 == -1)) exit(EXIT_FAILURE);

	users_id = msgget(key1, IPC_CREAT | 00666);
	chat_id = msgget(key2, IPC_CREAT | 00666);
	if ((users_id == -1) || (chat_id == -1)) exit(EXIT_FAILURE);

	pthread_create(&(tid), NULL, usersnum, &(users_id));


	while(working){
	}
	
	pthread_cancel(tid);
	exit(EXIT_SUCCESS);
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
			printf("I see user: %s My USER_NUM = %li\n", users[i].user_name, USERS_NUM);
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
		printf("----------------------------------\n");
	}

	free(users);
	pthread_exit(EXIT_SUCCESS);
}