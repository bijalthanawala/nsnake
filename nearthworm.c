/* Includes
 **************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include <ncurses.h>


/* Macros / Defnitions
 ************************/
#define DEFAULT_INIT_LENGTH 1 
#define DEFAULT_DRAW_CHAR  'S' 
#define DEFAULT_ERASE_CHAR  ' ' 
#define DEFAULT_TRACE_CHAR '.'
#define DEFAULT_FOOD_CHAR '@' 

#define ASCII_ESC	  27
#define ASCII_ARROW_UP    259
#define ASCII_ARROW_DN    258
#define ASCII_ARROW_LEFT  260
#define ASCII_ARROW_RIGHT 261

#define DEFAULT_DELAY (ONE_MILLI_SECOND * 100)
#define MIN_THRESHOLD_DELAY ( ONE_MILLI_SECOND * 20 )
#define ONE_MILLI_SECOND (1000)

#define SPEED_INC (-10) 
#define SPEED_DEC (+10) 

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
	chtype ch_draw;
	chtype ch_erase;
	bool b_show_segcount;
	bool b_show_length;
} SETTINGS, *P_SETTINGS;

typedef struct food {
	bool b_eaten;
	COORD coord;
} FOOD, *P_FOOD; 

/* Prototypes
 ****************/
WINDOW* ncurses_init();
void ncurses_uninit();

void init_settings(P_SETTINGS pset);
P_SNAKE snake_init();
void snake_draw_init(WINDOW *w, P_SETTINGS pset, P_SNAKE psnake);
bool snake_move(WINDOW *w, P_SETTINGS pset, P_SNAKE psnake, P_FOOD pfood);
bool snake_steer(WINDOW *w, P_SNAKE psnake, direction_t dir);

bool process_char(int ch, WINDOW *w, P_SETTINGS psettings, P_SNAKE psnake);

bool place_food(WINDOW *w , P_FOOD pfood);

void free_snake(P_SNAKE psnake);

/* Routines
 *************/
int main()
{
	WINDOW *w = NULL;
	P_SNAKE psnake = NULL;
	FOOD food;
	int ch = 0;
	SETTINGS settings;

	/* Initialize settings */
	init_settings(&settings);

	w = ncurses_init();
	
	psnake = snake_init(w);
	if( ! psnake ) {
		ncurses_uninit();
		return 1;
	}

	/* Draw the initial snake */
	snake_draw_init(w, &settings, psnake);

	food.b_eaten = true;
	while ( !(ch == 'x' || 
                  ch == 'q' ||
                  ch == ASCII_ESC 
                 )) {

		if(food.b_eaten) {
			place_food(w, &food);
		}
		usleep(settings.delay);
		if(!snake_move(w, &settings, psnake, &food)) {
			break;
		}
		ch = tolower(getch());
		process_char(ch, w, &settings, psnake);
	}
	
	ncurses_uninit();
	free_snake(psnake);
	return 0;
}

bool place_food(WINDOW *w , P_FOOD pfood)
{
	long int rnd = 0;
	P_COORD coord = &pfood->coord;

	pfood->b_eaten = false;

	srandom(time(NULL));		
	coord->y = random() % w->_maxy;  
	coord->x = random() % w->_maxx;  

	/* TODO: Detect and correct if food has landed on any part of snake itself */

	mvwaddch(w, coord->y, coord->x, DEFAULT_FOOD_CHAR);
}

bool process_char(int ch, WINDOW *w, P_SETTINGS pset, P_SNAKE psnake)
{
	switch(ch) 
	{
		case '-':	
			pset->delay += SPEED_DEC;
			break;	
		case '+':
			if ( (pset->delay + SPEED_INC) >= MIN_THRESHOLD_DELAY) {
				pset->delay += SPEED_INC;
			}
			break;	
		case 't':
			pset->ch_erase = (pset->ch_erase == DEFAULT_ERASE_CHAR) ? 
                                           DEFAULT_TRACE_CHAR :
				           DEFAULT_ERASE_CHAR;
			break;
		case 's':
			pset->b_show_segcount = pset->b_show_segcount ? false : true; 
			if(pset->b_show_segcount) 
				pset->b_show_length = false;
			break;
		case 'l':
			pset->b_show_length = pset->b_show_length ? false : true; 
			if(pset->b_show_length) 
				pset->b_show_segcount = false;
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
	
	/* TODO: Detect if head touched any part of snake itself */

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
	pseg->length = 0;	
	pseg->dir = dir;
	pseg->coord_start.x = head->coord_start.x;
	pseg->coord_start.y = head->coord_start.y; 
	pseg->coord_end.x = head->coord_start.x;
	pseg->coord_end.y = head->coord_start.y;
	seg_update_tailxy(pseg);

	/* Fix snake's old head segment */
	head->previous = pseg;

	/* Add new segment to the snake's front */
	psnake->seg_head = pseg;

	psnake->seg_count++;

	return true;
}

bool eat_food(P_SNAKE psnake, P_FOOD pfood)
{
	P_COORD coord_head = &psnake->seg_head->coord_start;
	P_COORD coord_food = &pfood->coord;

	if(coord_head->y == coord_food->y &&
	   coord_head->x == coord_food->x) {
		pfood->b_eaten = true;
		return true;
	}

	return false;
}

bool snake_move(WINDOW *w, P_SETTINGS pset, P_SNAKE psnake, P_FOOD pfood)
{
	P_SSEG head = psnake->seg_head;
	P_SSEG tail = psnake->seg_tail;
        chtype ch = pset->b_show_segcount ?  (psnake->seg_count + '0') :
		    	pset->b_show_length ? head->length + '0'  : pset->ch_draw;	

	seg_update_headxy(head);
	head->length++;
	if(is_valid_coord(w, &head->coord_start)) {
		mvwaddch(w, 
                       	head->coord_start.y, 
                        head->coord_start.x, 
			ch);
	}
	else
	{
		beep();
		return false;
	}

	if(eat_food(psnake, pfood)) {
		return true;
	}

	mvwaddch(w, tail->coord_end.y, tail->coord_end.x, pset->ch_erase);
	tail->length--;
	if ( tail->length == 0 ) {
		psnake->seg_tail = tail->previous;	
		psnake->seg_tail->next = NULL;
		psnake->seg_count--;
		free(tail);
	} 
	else {
		seg_update_tailxy(tail);
	}

	return true;
}

void init_settings(P_SETTINGS pset) 
{
	pset->delay = DEFAULT_DELAY;
	pset->ch_draw = DEFAULT_DRAW_CHAR;
	pset->ch_erase = DEFAULT_ERASE_CHAR;
	pset->b_show_segcount = false;
        pset->b_show_length = false;
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

	/* Initialize snake */
	psnake->seg_count = 1;

	/* Insert initial segment into the snake */
	psnake->seg_head = p_initseg;
	psnake->seg_tail = p_initseg;

	return psnake;
}

void snake_draw_init(WINDOW *w, P_SETTINGS pset, P_SNAKE psnake)
{
	int i;
	P_SSEG pseg = psnake->seg_head; 
	NCURSES_SIZE_T y = pseg->coord_start.y;
	NCURSES_SIZE_T x = pseg->coord_start.x;

	//mvwprintw(w, 1,1, "y=%d x=%d maxy=%d maxx=%d", y, x, w->_maxy, w->_maxx);

	for(i=0; i < pseg->length; i++, y++) {
		mvwaddch(w, y, x, pset->ch_draw);
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

void free_snake(P_SNAKE psnake)
{
	P_SSEG seg = psnake->seg_head;
	P_SSEG next = NULL;
	int count = 0;

	while(seg) {
		next = seg->next;
		seg = next;
	}

	free(psnake);
}
