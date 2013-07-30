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

#define ASCII_ESC	  27
#define ASCII_ARROW_UP    259
#define ASCII_ARROW_DN    258
#define ASCII_ARROW_LEFT  260
#define ASCII_ARROW_RIGHT 261

#define DEFAULT_DELAY HALF_A_SECOND
#define MIN_THRESHOLD_DELAY ( 100 * 1000 )
#define ONE_SECOND    (1000 * 1000)
#define HALF_A_SECOND (ONE_SECOND / 2)
#define QUARTER_SECOND (ONE_SECOND / 4)

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
typedef struct coord {
        NCURSES_SIZE_T x;	
        NCURSES_SIZE_T y;	
} COORD, *P_COORD;
	
typedef struct snake_segment {
	struct snake_segment *next;
	struct snake_segment *previous;
	direction_t dir;
	int length;
	COORD coord_start;
	COORD coord_end;
} SSEG, *P_SSEG;

typedef struct snake {
	int seg_count;
	P_SSEG seg_head;
	P_SSEG seg_tail; 
} SNAKE , *P_SNAKE;

typedef struct settings {
	int delay;
} SETTINGS, *P_SETTINGS;

/* Prototypes
 ****************/
WINDOW* ncurses_init();
void ncurses_uninit();

P_SNAKE snake_init();
void snake_draw_init(WINDOW *w, P_SSEG pseg);
bool snake_move(WINDOW *w, P_SNAKE psnake);
bool snake_steer(WINDOW *w, P_SNAKE psnake, direction_t dir);

bool process_char(int ch, WINDOW *w, P_SETTINGS psettings, P_SNAKE psnake);

/* Routines
 *************/
int main()
{
	WINDOW *w = NULL;
	P_SNAKE psnake = NULL;
	int ch = 0;
	SETTINGS settings;

	/* Initialize settings */
	settings.delay = DEFAULT_DELAY;

	w = ncurses_init();
	
	psnake = snake_init(w);
	if( ! psnake ) {
		ncurses_uninit();
		return 1;
	}

	while ( !(ch == 'x' || 
                  ch == 'q' ||
                  ch == ASCII_ESC 
                 )) {
		usleep(settings.delay);
		snake_move(w, psnake);
		ch = tolower(getch());
		process_char(ch, w, &settings, psnake);
	}
	
	ncurses_uninit();
	return 0;
}

bool process_char(int ch, WINDOW *w, P_SETTINGS psettings, P_SNAKE psnake)
{
	switch(ch) 
	{
		case '-':	
			psettings->delay += QUARTER_SECOND;
			break;	
		case '+':
			if ( psettings->delay - QUARTER_SECOND >= MIN_THRESHOLD_DELAY) {
				psettings->delay -= QUARTER_SECOND;
			}
			break;	
		case ASCII_ARROW_LEFT:
			snake_steer(w, psnake, DIR_LEFT);
			break;
		case ASCII_ARROW_RIGHT:
			snake_steer(w, psnake, DIR_RIGHT);
			break;
		case ASCII_ARROW_UP:
			snake_steer(w, psnake, DIR_UP);
			break;
		case ASCII_ARROW_DN:
			snake_steer(w, psnake, DIR_DOWN);
			break;
	}
}

bool seg_update_coord(direction_t dir, P_COORD pcoord)
{
	switch(dir) {
		case DIR_LEFT:
			pcoord->x--;
			break;
		case DIR_RIGHT:
			pcoord->x++;
			break;
		case DIR_UP:
			pcoord->y--;
			break;
		case DIR_DOWN:
			pcoord->y++;
			break;
	}
}

bool seg_update_headxy(P_SSEG seg)
{
	seg_update_coord(seg->dir, &seg->coord_start);
}

bool seg_update_tailxy(P_SSEG seg)
{
	seg_update_coord(seg->dir, &seg->coord_end);
}

bool is_valid_coord(WINDOW *w, P_COORD pcoord)
{
	if(pcoord->x >= w->_begx && 
 	   pcoord->x <= w->_maxx &&
	   pcoord->y >= w->_begy &&
	   pcoord->y <= w->_maxy) { 
		return true;
	}

	return false;
}

bool snake_steer(WINDOW *w, P_SNAKE psnake, direction_t dir)
{
	P_SSEG pseg = NULL;
	P_SSEG head = psnake->seg_head;
	bool dir_ok = true;

	if( psnake->seg_head->dir == dir )
		return true;
	
	
	//mvprintw(0,0,"dir=%x head-dir=%x CHECKING", dir, psnake->seg_head->dir);
	switch(dir) 
	{
	case DIR_UP:
		if(psnake->seg_head->dir == DIR_DOWN) {
			dir_ok = false;	
		}
		break;
	case DIR_DOWN:
		if(psnake->seg_head->dir == DIR_UP) {
			dir_ok = false;	
		}
		break;
	case DIR_LEFT:
		if(psnake->seg_head->dir == DIR_RIGHT) {
			dir_ok = false;	
		}
		break;
	case DIR_RIGHT:
		if(psnake->seg_head->dir == DIR_LEFT) {
			dir_ok = false;	
		}
		break;
	}
	//mvprintw(0,0,"dir=%x head-dir=%x %s", dir, psnake->seg_head->dir, dir_ok ? "PASSED" : "FAILED");
	if( ! dir_ok ) {
		return false;
	}
	
	pseg = calloc(sizeof(SSEG), 1);
	if( ! pseg) {
		return false;
	}
	
	/* TODO: Handle case where steering is catastrophic */
	
	/* Setup the new segment in the expected direction */
	pseg->previous = NULL;
	pseg->next = psnake->seg_head;
	pseg->length = 1;	
	pseg->dir = dir;
	pseg->coord_start.x = head->coord_start.x;
	pseg->coord_start.y = head->coord_start.y; 
	pseg->coord_end.x = head->coord_start.x;
	pseg->coord_end.y = head->coord_start.y;

	/* Fix snake's old head segment */
	head->previous = pseg;

	/* Add new segment to the snake's front */
	psnake->seg_count++;
	psnake->seg_head = pseg;

	return true;
}

bool snake_move(WINDOW *w, P_SNAKE psnake)
{
	P_SSEG head = psnake->seg_head;
	P_SSEG tail = psnake->seg_tail;

	seg_update_headxy(head);
	head->length++;
	if(is_valid_coord(w, &head->coord_start)) {
		mvwaddch(w, 
                       	head->coord_start.y, 
                        head->coord_start.x, 
			DEFAULT_SNAKE_CHAR);
	}
	else
	{
		beep();
		return false;
	}

	//mvwinsch(w, tail->coord_end.y, tail->coord_end.x, '.');
        mvwdelch(w, tail->coord_end.y, tail->coord_end.x);	
	tail->length--;
	if ( tail->length == 0 ) {
		psnake->seg_tail = tail->previous;	
	} 
	else {
		seg_update_tailxy(tail);
	}

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
	p_initseg->previous = NULL;
	p_initseg->next = NULL;
	p_initseg->length = DEFAULT_INIT_LENGTH;	
	p_initseg-> dir = DIR_UP;
	p_initseg->coord_start.x = w->_maxx;
	p_initseg->coord_start.y = w->_maxy - DEFAULT_INIT_LENGTH + 1;
	p_initseg->coord_end.x = w->_maxx;
	p_initseg->coord_end.y = w->_maxy;

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
	NCURSES_SIZE_T y = pseg->coord_start.y;
	NCURSES_SIZE_T x = pseg->coord_start.x;

	//mvwprintw(w, 1,1, "y=%d x=%d maxy=%d maxx=%d", y, x, w->_maxy, w->_maxx);

	for(i=0; i < pseg->length; i++, y++) {
		mvwinsch(w, y, x, DEFAULT_SNAKE_CHAR);
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

