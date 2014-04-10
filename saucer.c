	/*
	 * tanimate.c: animate several strings using threads, curses, usleep()
	 *
	 *	bigidea one thread for each animated string
	 *		one thread for keyboard control
	 *		shared variables for communication
	 *	compile	cc tanimate.c -lcurses -lpthread -o tanimate
	 *	to do   needs locks for shared variables
	 *	        nice to put screen handling in its own thread
	 */

#include	<stdio.h>
#include	<curses.h>
#include	<pthread.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>


#define	MAX_NSAUCERS	10		/* limit to number of strings	*/
#define	TUNIT   20000		/* timeunits in microseconds */
#define MAXLINES 5      /* limit for the number of lines saucer occupies */
#define MAX_NROCKETS 25

struct	propset {
		char	str [128];	/* the message */
		int	row;	/* the row     */
		int	delay;  /* delay in time units */
		int	dir;	/* +1 or -1	*/
		int col;
	
	};

const char *saucerz[4] = {"<--->", "<+++>", "<~~~>", "<***>"};
const char *launch = "|";
const char *rocket = "^";

struct propset rockets[MAX_NROCKETS];	/* properties of string	*/
struct propset saucers[MAX_NSAUCERS];
struct propset launchProp;	/* properties of string	*/

pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

int saucersEscaped = 0;
int score = 0;
int nRockets = MAX_NROCKETS;
int gameOver = 0;



int collides(struct propset a, struct propset b) {
		if (
			( (b.col >= a.col && b.col <= a.col + strlen(a.str)) ||
			 (a.col >= b.col && a.col <= b.col + strlen(b.str)) ) &&
			    (b.row == a.row  )
				) return 1;
			return 0;
	}

void moveUp(char * string, int r, int c){
		pthread_mutex_lock(&mx);	/* only one thread	*/
		move( r, c );	/* can call curses	*/
		addch(' ');			/* at a the same time	*/
		addstr( string );		/* Since I doubt it is	*/
		addch(' ');			/* reentrant		*/
		move(r+1, c);
		hline(' ', 5);
		move(LINES-1,COLS-1);	/* park cursor		*/
		refresh();			/* and show it		*/
		pthread_mutex_unlock(&mx);	/* only one thread	*/
	}

void moveRight (char * string, int r, int c){
		
		pthread_mutex_lock(&mx);	/* only one thread	*/
		move( r, c );	/* can call curses	*/
		addch(' ');			/* at a the same time	*/
		addstr( string );		/* Since I doubt it is	*/
		addch(' ');			/* reentrant		*/
		move(LINES-1,COLS-1);	/* park cursor		*/
		refresh();			/* and show it		*/
		pthread_mutex_unlock(&mx);	/* only one thread	*/
	}


	/* the code that runs in each thread */
void *animateSaucer(void *arg){
		struct propset *info = arg;	/* properties of string	*/
		
		int sType = (rand()%4);
		//memcpy (props, p, sizeof(props));
		memcpy (info->str, saucerz[sType], strlen(saucerz[sType])+1);	/* the message	*/
		info->row = (rand()%MAXLINES);		/* the row	*/
		info->delay = (TUNIT * (2 + ((rand()%15 ) - (score/5))));	/* a speed	*/
		if (info->delay < 2 * TUNIT){
			info->delay = 2;
		}
		info->dir = +1;	/* +1 or -1	*/
		
		//printf ("inside animateSaucer");
		char tmp [10] = "     ";
		//struct propset *info = arg;		/* point to info block	*/
		int	len = strlen(info->str)+2;	/* +2 for padding	*/
		info->col = 2;	/* space for padding	*/
		
		while( !gameOver )
		{
			usleep(abs(info->delay));
			moveRight (info->str, info->row, info->col);
			
			/* move item to next column and check for bouncing	*/
			pthread_mutex_lock(&mx);
			info->col += info->dir;
			pthread_mutex_unlock(&mx);
			if ( (info->col+len >= COLS) || (info->delay < 0) ){
				memcpy (info->str, tmp, strlen(tmp) +1);
				moveRight (info->str, info->row, info->col);
				pthread_mutex_lock(&mx);
				if (info->delay < 0){
					score += 1;
					nRockets +=1;
				}
				else saucersEscaped += 1;
				
				info->delay = -1;
				pthread_mutex_unlock(&mx);	/* only one thread	*/
				pthread_exit(NULL);
			}
			
			
			
		}
		pthread_exit(NULL);
		
	}


void *createSaucer(void *arg){
		//printf ("inside saucer");
		while (!gameOver) {
			
			
			//(rand()%(max-min))+min;
			usleep(((rand()%50) * (10/(1+score)) * TUNIT) + (100 *TUNIT));
			int	       c;		/* user input		*/
			pthread_t      thrd;	/* the threads		*/
			int i;
			
			/* create the thread */
			//pthread_create(&pthread, NULL, pthread_handler, (void *)pdata);
			for (i = 0; i < MAX_NSAUCERS ; i ++){
				if (saucers[i].delay < 0 ){
					//int index = i;
					if ( pthread_create(&thrd, NULL, animateSaucer, &saucers[i])){
						fprintf(stderr,"error creating thread");
						endwin();
						exit(0);
					}
					break;
				}
			}
			
		}
		pthread_exit(NULL);
	}

void *animateLaunchSite (void *arg){
		
		struct propset *info = arg;
		
		
		
		int	len = strlen(info->str)+2;	/* +2 for padding	*/
		info->col = (COLS/2);	/* space for padding	*/
		info->row = LINES - 3;
		
		while( !gameOver )
		{
			usleep(TUNIT);
			
			
			pthread_mutex_lock(&mx);	/* only one thread	*/
			move( info->row, info->col );	/* can call curses	*/
			addch(' ');			/* at a the same time	*/
			addstr( info->str );		/* Since I doubt it is	*/
			addch(' ');			/* reentrant		*/
			move(LINES-1,COLS-1);	/* park cursor		*/
			refresh();			/* and show it		*/
			
			/* move item to next column and check for bouncing	*/
			
			info->col += info->dir;
			pthread_mutex_unlock(&mx);	/* done with curses	*/
			
			
			if ( info->col <= 0 && info->dir == -1 ){
				info->dir = 1;
				continue;
			}
			else if (  info->col+len >= COLS && info->dir == 1 ){
				info->dir = -1;
				continue;
			}
			info->dir = 0;
		}
		pthread_exit(NULL);
	}

void *animateRocket (void *arg){
		
		struct propset *info = arg;
		//struct propset info;
		info->dir = -1;
		
		pthread_mutex_lock(&mx);	/* only one thread	*/
		info->row = launchProp.row;
		info->col = launchProp.col;
		pthread_mutex_unlock(&mx);	/* done with curses	*/
		
		memcpy(info->str, rocket, strlen(rocket)+1);
		info->delay = 2* TUNIT;
		
		int	len = strlen(info->str)+2;	/* +2 for padding	*/
		char tmp [3] = "  ";
		
		while( !gameOver )
		{
			usleep(abs(info->delay));
			int i = 0;
			moveUp(info->str, info->row, info->col);
			
			
			/* move item to next column and check for bouncing	*/
			
			info->row += info->dir;
			
			struct propset p;
			p.row = info->row;
			p.col = info->col;
			memcpy (p.str, info->str, strlen(info->str) +1);
			for (i = 0; i < MAX_NSAUCERS; i++) {
				pthread_mutex_lock(&mx);
				
				if ((collides (saucers[i], p)) && (saucers[i].delay >= 0)){
					memcpy (info->str, tmp, strlen(tmp) +1);
					
					move( info->row, info->col );	/* can call curses	*/
					addch(' ');			/* at a the same time	*/
					addstr( info->str );		/* Since I doubt it is	*/
					addch(' ');			/* reentrant		*/
					move(info->row+1, info->col);
					hline (' ', 5);
					move(LINES-1,COLS-1);	/* park cursor		*/
					refresh();			/* and show it		*/
					info->delay = -1;
					saucers[i].delay = -1;
					
					pthread_mutex_unlock(&mx);
					pthread_exit(NULL);
				}
				pthread_mutex_unlock(&mx);
			}
			
			
			
			if ( info->row <= 0){
				
				memcpy (info->str, tmp, strlen(tmp) +1);
				moveUp(info->str, info->row, info->col);
				pthread_mutex_lock(&mx);
				info->delay = -1;
				if (nRockets <= 0) {
					gameOver  = 1;
				}
				pthread_mutex_unlock(&mx);
				pthread_exit(NULL);
			}
		}
		pthread_exit(NULL);
	}

void *game (void* arg){
		int oldscore;
		int oldsaucersEscaped;
		int oldnRockets;
		while (1){
			
			if (score!= oldscore || saucersEscaped != oldsaucersEscaped ||
				nRockets!= oldnRockets) {
				pthread_mutex_lock(&mx);
				mvprintw(LINES-1,0,"'Q' to quit, You have '%d' rockets, "
					"score: '%d', escaped saucers: '%d'",nRockets, score, 
					saucersEscaped);
				pthread_mutex_unlock(&mx);
				oldscore = score;
				oldnRockets = nRockets;
				oldsaucersEscaped = saucersEscaped;

			}
			

			if (saucersEscaped > 5) {gameOver = 1;}
			
			if (gameOver){
				//sleep(1);
				wclear(stdscr);
				refresh();
				
				mvprintw(LINES/2,(COLS/2)-5,"GAMEOVER!");
				mvprintw((LINES/2)+1,(COLS/2)-12,"PRESS ANY BUTTON TO EXIT");
				mvprintw(LINES-1,0,"score: '%d', escaped saucers: '%d'", 
					score, saucersEscaped);
				pthread_mutex_unlock(&mx);
				break;
			}			
			
		}
		pthread_exit(NULL);
	}



int main(int ac, char *av[]){
		
		int	       c;		/* user input		*/
		pthread_t      sThrds, lThrd, rThrd, gThrd;	/* the threads		*/
		//void	       *createSaucer();	/* the function		*/
		int i;
		
		memcpy (launchProp.str, launch, strlen(launch)+1);
		launchProp.delay = TUNIT;	/* a speed	*/
		launchProp.dir = 0;	/* +1 or -1	*/
		
		
		/* setup */
		for (i = 0; i< MAX_NSAUCERS; i++) {
			saucers[i].delay = -1;
		}
		
		for (i = 0; i< MAX_NROCKETS; i++) {
			rockets[i].delay = -1;
		}
		initscr();
		crmode();
		noecho();
		clear();
		mvprintw(LINES-1,0,"'Q' to quit, You have '%d' rockets, score: " 
			"'%d', escaped saucers: '%d'",nRockets, score, saucersEscaped);
		
		
		/* create all the threads */
		if ( pthread_create(&gThrd, NULL, game, 0)){
			fprintf(stderr,"error creating thread");
			endwin();
			exit(0);
		}
		
		if ( pthread_create(&lThrd, NULL, animateLaunchSite, &launchProp)){
			fprintf(stderr,"error creating thread");
			endwin();
			exit(0);
		}
		
		if ( pthread_create(&sThrds, NULL, createSaucer, 0)){
			fprintf(stderr,"error creating thread");
			endwin();
			exit(0);
		}
		
		
		
		
		/* process user input */
		while(!gameOver) {
			
			c = getchar();
			if ( c == 'Q' ) break;
			
			if ( c == '9' )
				launchProp.dir = -1;
			if ( c == '0' )
				launchProp.dir = 1;
			if ( c == ' ' ){
				pthread_mutex_lock(&mx);	/* only one thread	*/
				if ((nRockets <= MAX_NROCKETS) && (nRockets > 0)) {
					
					for(i = 0; i < MAX_NROCKETS; i++){
						if (rockets[i].delay < 0) {
							
							
							if ( pthread_create(&rThrd, NULL, animateRocket, 
								&rockets[i])){
								fprintf(stderr,"error creating thread");
								endwin();
								exit(0);
							}
							else {
								nRockets += -1;
							}
							break;
						}
					}
					
				}
				pthread_mutex_unlock(&mx);
				
			}
			//pthread_mutex_lock(&mx);

			pthread_mutex_unlock(&mx);
			
		}
		
		/* cancel all the threads */
		pthread_mutex_lock(&mx);
		
		pthread_cancel(lThrd);
		endwin();
		if (gameOver) printf ("\n\nGAMEOVER! \nscore: '%d', "
			"escaped saucers: '%d'\n", score, saucersEscaped);

		return 0;
}
