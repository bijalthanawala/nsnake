/* Includes
 **************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ncurses.h>


/* Macros / Defnitions
 ************************/
#define DEFAULT_INIT_LENGTH 5
#define DEFAULT_SNAKE_CHAR  'S' 

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
	direction_t dir;
	int length;
        NCURSES_SIZE_T currx;	
        NCURSES_SIZE_T curry;	
} SSEG, *P_SSEG;

/* Prototypes
 ****************/
WINDOW* ncurses_init();
void ncurses_uninit();

bool snake_init();
void snake_draw_init(P_SSEG pseg);

/* Routines
 *************/
int main()
{
	WINDOW *w = NULL;

	w = ncurses_init();
	
	if(! snake_init(w)) {
		return 1;
	}
	
	getch();

	ncurses_uninit();
	return 0;
}

bool snake_init(WINDOW *w)
{
	P_SSEG p_initseg = calloc(sizeof(SSEG), 1);
	if( ! p_initseg) {
		return false;
	}
	
	/* TODO: Handle case where maxx/maxy of ncurses window
                 is not large enough to hold the initial size of
                 the snake
	*/
	
	/* Initialize the very first segment */
	p_initseg->length = DEFAULT_INIT_LENGTH;	
	p_initseg-> dir = DIR_UP;
	p_initseg->currx = w->_maxx;
	p_initseg->curry = w->_maxy - DEFAULT_INIT_LENGTH - 1;

	snake_draw_init(p_initseg);

	return true;
}

void snake_draw_init(P_SSEG pseg)
{
	int i;
	NCURSES_SIZE_T y = pseg->curry;
	NCURSES_SIZE_T x = pseg->currx;

	for(i=0; i < pseg->length; i++, y++) {
		mvaddch(y, x, DEFAULT_SNAKE_CHAR);
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
	
	return w;
}


void ncurses_uninit()
{
	endwin();
}

