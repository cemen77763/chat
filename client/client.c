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

int USERS_NUM = 0;

struct msgbuf_user{
	long mtype;
	char user_name[30];	
	long num;
};

struct pdata{
	struct msgbuf_user user;
	WINDOW *win;
	int id;
};

void sig_winch(int signo);

void menu(WINDOW *usersWin, WINDOW *chatWin);

void *usersnum(void *pthread_data);

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
    printw("Enter to send message");
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

void client(WINDOW *chatWin, WINDOW *usersWin){
	key_t key1;
	int working = 1, user_id;
	int ch;
	pthread_t tid;
	struct pdata data;

	key1 = 1234;
	data.id = msgget(key1, IPC_CREAT | 00666);

	data.win = usersWin;

	takename(chatWin, data.user.user_name);
	wclear(chatWin);
	menu(usersWin, chatWin);

	pthread_create(&(tid), NULL, usersnum, &(data));

	while(ch != 10){
		ch = getch();
	}
	pthread_cancel(tid);
}