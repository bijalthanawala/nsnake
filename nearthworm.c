/* Includes
 **************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>

#include <ncurses.h>


/* Macros / Defnitions
 ************************/
#define DEFAULT_INIT_LENGTH 5
#define DEFAULT_SNAKE_CHAR  'S' 

#define ASCII_ARROW_UP    259
#define ASCII_ARROW_DN    258
#define ASCII_ARROW_LEFT  260
#define ASCII_ARROW_RIGHT 261

#define DEFAULT_DELAY HALF_A_SECOND
#define ONE_SECOND    (1000 * 1000)
#define HALF_A_SECOND (ONE_SECOND / 2)

/* enums
 ***********/
 typedef enum  { 
		DIR_LEFT  = 0xA000,
 		DIR_RIGHT = 0xA002,
 		DIR_UP    = 0xA004,
 		DIR_DOWN  = 0xA006
	} direction_t;

/* Structures
 *******************/
typedef struct snake_segment {
	struct snake_segment *next;
	direction_t dir;
	int length;
        NCURSES_SIZE_T currx;	
        NCURSES_SIZE_T curry;	
        NCURSES_SIZE_T endx;	
        NCURSES_SIZE_T endy;	
} SSEG, *P_SSEG;

typedef struct snake {
	int seg_count;
	P_SSEG seg_head;
	P_SSEG seg_tail; 
} SNAKE , *P_SNAKE;

/* Prototypes
 ****************/
WINDOW* ncurses_init();
void ncurses_uninit();

P_SNAKE snake_init();
void snake_draw_init(WINDOW *w, P_SSEG pseg);
bool snake_move(WINDOW *w, P_SNAKE psnake);

/* Routines
 *************/
int main()
{
	WINDOW *w = NULL;
	P_SNAKE psnake = NULL;
	int ch = 0;

	w = ncurses_init();
	
	psnake = snake_init(w);
	if( ! psnake ) {
		ncurses_uninit();
		return 1;
	}

	while ( !(ch == 'x' || 
                  ch == 'q' ||
                  ch == 27 
                 )) {
		usleep(DEFAULT_DELAY);
		ch = tolower(getch());
		snake_move(w, psnake);
	}
	
	ncurses_uninit();
	return 0;
}

bool seg_update_headxy(P_SSEG seg)
{
	switch(seg->dir) {
		case DIR_LEFT:
			seg->currx--;
			break;
		case DIR_RIGHT:
			seg->currx++;
			break;
		case DIR_UP:
			seg->curry--;
			break;
		case DIR_DOWN:
			seg->curry++;
			break;
	}
}

bool seg_update_tailxy(P_SSEG seg)
{
	switch(seg->dir) {
		case DIR_LEFT:
			seg->endx--;
			break;
		case DIR_RIGHT:
			seg->endx++;
			break;
		case DIR_UP:
			seg->endy--;
			break;
		case DIR_DOWN:
			seg->endy++;
			break;
	}
}

bool snake_move(WINDOW *w, P_SNAKE psnake)
{
	P_SSEG head = psnake->seg_head;
	P_SSEG tail = psnake->seg_tail;

	seg_update_headxy(head);
	head->length++;
	//mvwprintw(w, 1,1, "curry=%d currxx=%d maxy=%d maxx=%d", head->curry, head->currx, w->_maxy, w->_maxx);
	if(head->currx >= w->_begx && 
 	   head->currx <= w->_maxx &&
	   head->curry >= w->_begy &&
	   head->curry <= w->_maxy) { 
		mvwinsch(w, head->curry, head->currx, DEFAULT_SNAKE_CHAR);
	}
	else
	{
		beep();
		return false;
	}

	mvwinsch(w, tail->endy, tail->endx, '.');
        mvwdelch(w, tail->endy, tail->endx);	
	tail->length--;
	seg_update_tailxy(tail);

	return true;
}

P_SNAKE snake_init(WINDOW *w)
{
	P_SNAKE psnake = NULL;
	P_SSEG p_initseg = NULL;

	psnake = calloc(sizeof(SNAKE),1);
	if( ! psnake) {
		return NULL;
	}
	
	p_initseg = calloc(sizeof(SSEG), 1);
	if( ! p_initseg) {
		free(psnake);
		return NULL;
	}
	
	/* TODO: Handle case where maxx/maxy of ncurses window
                 is not large enough to hold the initial size of
                 the snake
	*/
	
	/* Initialize the very first segment */
	p_initseg->next = NULL;
	p_initseg->length = DEFAULT_INIT_LENGTH;	
	p_initseg-> dir = DIR_UP;
	p_initseg->currx = w->_maxx;
	p_initseg->curry = w->_maxy - DEFAULT_INIT_LENGTH + 1;
	p_initseg->endx = w->_maxx;
	p_initseg->endy = w->_maxy;

	/* Insert initial segment into the snake */
	psnake->seg_count = 1;
	psnake->seg_head = p_initseg;
	psnake->seg_tail = p_initseg;

	/* Draw the initial snake */
	snake_draw_init(w, p_initseg);

	return psnake;
}

void snake_draw_init(WINDOW *w, P_SSEG pseg)
{
	int i;
	NCURSES_SIZE_T y = pseg->curry;
	NCURSES_SIZE_T x = pseg->currx;

	//mvwprintw(w, 1,1, "y=%d x=%d maxy=%d maxx=%d", y, x, w->_maxy, w->_maxx);

	for(i=0; i < pseg->length; i++, y++) {
		mvwaddch(w, y, x, DEFAULT_SNAKE_CHAR);
	}
}

WINDOW* ncurses_init()
{
	WINDOW *w = NULL;
	w = initscr();
	keypad(stdscr, TRUE);
	cbreak();
	noecho();
	nonl();
	nodelay(w, true);
	
	return w;
}


void ncurses_uninit()
{
	endwin();
}

