#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curses.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

int USERS_NUM = 0;
int COL = 1;

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

struct pdata{
	struct msgbuf_user user;
	struct msgbuf_chat chat;
	WINDOW *win;
	int id;
};

void sig_winch(int signo);

void menu(WINDOW *usersWin, WINDOW *chatWin);

void *usersnum(void *pthread_data);

void takemess(char *buff);

void *usersmes(void *pthread_data);

void takename(WINDOW *chatWin, char *name);

void client(WINDOW *chatWin, WINDOW *usersWin);

void main(){
	WINDOW *usersWin;
	WINDOW *chatWin;

	initscr();
	signal(SIGWINCH, sig_winch);
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	curs_set(TRUE);
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	init_pair(3, COLOR_GREEN, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLUE);

	chatWin = newwin(20, 50, 0, 0);
	usersWin = newwin(20, 80, 0, 50);

	menu(usersWin, chatWin);
	client(chatWin, usersWin);

	delwin(chatWin); delwin(usersWin);
	endwin();
	exit(EXIT_SUCCESS);
}

void sig_winch(int signo){
	struct winsize size;
	ioctl(fileno(stdout), TIOCGWINSZ, (char*) &size);
	resizeterm(size.ws_row, size.ws_col);
}

void menu(WINDOW *usersWin, WINDOW *chatWin){
	move(20,0);      
    hline(ACS_CKBOARD,100); 
    move(21, 0);
    printw("Esc to quit");
	refresh();

	wbkgd(chatWin, COLOR_PAIR(1));
	wbkgd(usersWin, COLOR_PAIR(2));
	box(chatWin, '|', '-');
	box(usersWin, '|', '-');

	wmove(chatWin, 0, 1);
	wattron(chatWin, COLOR_PAIR(1));
	wprintw(chatWin, "Chat:");

	wmove(usersWin, 0, 1);
	wattron(usersWin, COLOR_PAIR(2));
	wprintw(usersWin, "Users:");

	wmove(chatWin, 18, 1);
	wattron(chatWin, COLOR_PAIR(4));
	wprintw(chatWin, "Enter message: ");
	wattron(chatWin, COLOR_PAIR(1));

	wrefresh(chatWin);
	wrefresh(usersWin);
}

void takename(WINDOW *chatWin, char *name){
	int ch = 0;
	int i = 34;
	wmove(chatWin, 1, 1);
	wprintw(chatWin, "Input your name and press enter: ");
	wrefresh(chatWin);

	while(ch != 10){
		ch = getch();
		if (ch == KEY_BACKSPACE){
			if (i > 33){
				i--;
				mvwaddstr(chatWin, 1, i, " ");
				wmove(chatWin, 1, i);
			}
		} else {
			mvwaddch(chatWin, 1, i, ch);
			wmove(chatWin, 1, i);
			i++;
		}
		wrefresh(chatWin);
	}

	wmove(chatWin, 1, 34);
	winstr(chatWin, name);
	i = 0;
	do{
	 	i++;
	} while(name[i] != ' ');
	name[i] = '\0';
}

void *usersnum(void *pthread_data){
	int working = 1, i = 0, k = 0;
	struct msgbuf_user *getuser;
	struct pdata *data = (struct pdata*)pthread_data;
	data->user.mtype = 1;
	msgsnd(data->id, &(data->user), sizeof(struct msgbuf_user), 0);
	msgrcv(data->id, &(data->user), sizeof(struct msgbuf_user), 2, 0);

	getuser = malloc(sizeof(struct msgbuf_user));
	
	while(1){
		while(working){
			getuser = realloc(getuser, sizeof(struct msgbuf_user)*(k + 1));
			msgrcv(data->id, &(getuser[i]), sizeof(struct msgbuf_user), data->user.num + 2, 0);

			if (getuser[i].num == 665){
				USERS_NUM = k;
				break;
			}
			i++; k++;
		}
		wattron(data->win, COLOR_PAIR(3));

		for (i = 0; i < k; i++){
			wmove(data->win, 1 + i, 1);
			wprintw(data->win, getuser[i].user_name);
			wrefresh(data->win);
		}
		wattron(data->win, COLOR_PAIR(2));
		i = 0;
		k = 0;
	}

	free(getuser);
	pthread_exit(EXIT_SUCCESS);
}

void takemess(char *buff){
	int i = 0;
	while((buff[i] != ' ') && (buff[i] != '|'))
		i++;
	buff[i + 1] = '\0';
}

void *usersmes(void *pthread_data){
	int working = 1;
	struct pdata *data = (struct pdata*)pthread_data;
	struct msgbuf_chat chat;

	while(working){
		if (COL >= 17){
			wclear(data->win);
			box(data->win, '|', '-');
			wmove(data->win, 18, 1);
			wattron(data->win, COLOR_PAIR(4));
			wprintw(data->win, "Enter message: ");
			wattron(data->win, COLOR_PAIR(1));
			COL = 1;
		}
		msgrcv(data->id, &(chat), sizeof(struct msgbuf_chat), data->chat.num, 0);
		wmove(data->win, COL, 1);
		wprintw(data->win, "%s: %s", chat.user_name, chat.message);
		wrefresh(data->win);
		COL++;
	}


	pthread_exit(EXIT_SUCCESS);
}

void client(WINDOW *chatWin, WINDOW *usersWin){
	key_t key1, key2;
	int working = 1, user_id;
	int ch, i = 0, j = 0;
	char buff[255];
	pthread_t tid[2];
	struct pdata data[2];

	key1 = 1234;
	key2 = 1233;
	data[0].id = msgget(key1, IPC_CREAT | 00666);
	data[1].id = msgget(key2, IPC_CREAT | 00666);

	data[0].win = usersWin;
	data[1].win = chatWin;

	takename(chatWin, data[0].user.user_name);
	do {
		data[1].chat.user_name[i] = data[0].user.user_name[i];
		i++;
	} while (data[0].user.user_name[i] != '\0');
	data[1].chat.user_name[i] = '\0';

	wclear(chatWin);
	menu(usersWin, chatWin);

	pthread_create(&(tid[0]), NULL, usersnum, &(data[0]));

	data[1].chat.mtype = 1;
	msgsnd(data[1].id, &(data[1].chat), sizeof(struct msgbuf_chat), 0);
	msgrcv(data[1].id, &(data[1].chat), sizeof(struct msgbuf_chat), 2, 0);
	pthread_create(&(tid[1]), NULL, usersmes, &(data[1]));

	i = 0;
	while(working){
		if (COL >= 17){
			wclear(chatWin);
			menu(usersWin, chatWin);
			i = 0;
			COL = 1;
		}
		ch = getch();
		switch(ch){
			case 10:{
				wmove(chatWin, 18, 16);
				winstr(chatWin, buff);

				j = 0;
				do {
					data[1].chat.message[j] = buff[j];
					j++;
				} while(buff[j + 1] != '|');
				data[1].chat.message[j] = '\0';

				i = 16;
				wmove(chatWin, 18, i);
				winstr(chatWin, buff);
				while(buff[0] != '|'){
					mvwaddstr(chatWin, 18, i, " ");
					i++;
					wmove(chatWin, 18, i);
					winstr(chatWin, buff);
				}
				i = 0;

				data[1].chat.mtype = 3;
				msgsnd(data[1].id, &(data[1].chat), sizeof(struct msgbuf_chat), 0);
				break;
			}
			case 27:{
				working = 0;
				break;
			}
			case KEY_BACKSPACE:
			{
				i--;
				mvwaddstr(chatWin, 18, 16 + i, " ");
				wmove(chatWin, 18, 16 + i);
				wrefresh(chatWin);
				break;
			}
			default:{
				wmove(chatWin, 18, 16 + i);
				mvwaddch(chatWin, 18, 16 + i, ch);
				wrefresh(chatWin);
				i++;
				break;
			}
		}
	}
	pthread_cancel(tid[0]); pthread_cancel(tid[1]);
}