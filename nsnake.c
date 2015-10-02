/* Includes
 **************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include <ncurses.h>


/* Macros / Defnitions
 ************************/
#define DEFAULT_INIT_LENGTH 15
#define DEFAULT_DRAW_CHAR  ' ' 
#define DEFAULT_ERASE_CHAR  ' ' 
#define DEFAULT_TRACE_CHAR '.'
#define DEFAULT_FOOD_CHAR '@' 

#define MIN_SPEED	1
#define MAX_SPEED	9
#define DEFAULT_SPEED	5
#define ONE_MILLI_SECOND (1000)
#define DELAY_DELTA ( ONE_MILLI_SECOND * 25 )

#define COLOR_PAIR_BOX   	1
#define COLOR_PAIR_FOOD 	2
#define COLOR_PAIR_SNAKE	3
#define COLOR_PAIR_STATUS	4
#define COLOR_PAIR_RED_ON_BLACK	5

#define STATUS_ATTR	  	(WA_BOLD | WA_UNDERLINE)
#define STATUS_SPEED_AVAIL	(WA_BOLD)
#define STATUS_DIR	  	(WA_BOLD | WA_BLINK)

//#define DRAW_CHAR(w, y, x, ch) mvwaddch(w, y, x, ch)
#define DRAW_CHAR(w, y, x, ch) mvaddch(y, x, ch)

#define DRAW_SNAKE_HEAD(w, y, x, ch) { \
	attron(COLOR_PAIR(COLOR_PAIR_SNAKE)); \
	DRAW_CHAR(w, y, x, ch); \
	attroff(COLOR_PAIR(COLOR_PAIR_SNAKE)); \
	}

/* enums
 ***********/
 typedef enum  { 
		DIR_LEFT  = 0xA000,
 		DIR_RIGHT = 0xA002,
 		DIR_UP    = 0xA004,
 		DIR_DOWN  = 0xA006,
		DIR_UP_LEFT    = 0xA008,
		DIR_UP_RIGHT   = 0xA00a,
		DIR_DOWN_LEFT  = 0xA00c,
		DIR_DOWN_RIGHT = 0xA00e
	} direction_t;

/* Structures
 *******************/
typedef struct snake_window {
       NCURSES_SIZE_T _maxy, _maxx;
       NCURSES_SIZE_T _begy, _begx;
} WINDOW_SNAKE, *P_WINDOW_SNAKE;

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
	int score;
	P_SSEG seg_head;
	P_SSEG seg_tail; 
} SNAKE , *P_SNAKE;

typedef struct settings {
	bool b_altered;
	int  speed;
	bool pause;
	bool portal;
	bool cheat;
	bool reverse;
	bool sound;
	chtype ch_draw;
	chtype ch_erase;
	chtype ch_food;
	bool b_show_segcount;
	bool b_show_length;
} SETTINGS, *P_SETTINGS;

typedef struct food {
	bool b_eaten;
	COORD coord;
} FOOD, *P_FOOD; 

/* Prototypes
 ****************/
WINDOW* ncurses_init(P_WINDOW_SNAKE p_ws);
void ncurses_uninit();

void init_settings(P_SETTINGS pset);
P_SNAKE snake_init(WINDOW_SNAKE *ws);
void free_snake(P_SNAKE psnake);
void snake_draw_init(WINDOW_SNAKE *ws, P_SETTINGS pset, P_SNAKE psnake);

bool snake_move(WINDOW_SNAKE *ws, P_SETTINGS pset, P_SNAKE psnake, P_FOOD pfood);
bool snake_steer(WINDOW_SNAKE *ws, P_SETTINGS pset,  P_SNAKE psnake, direction_t dir);
bool place_food(WINDOW_SNAKE *ws , P_SETTINGS pset, P_FOOD pfood, P_SNAKE psnake);
bool eat_food(P_SETTINGS pset, P_SNAKE psnake, P_FOOD pfood);
bool is_self_collision(P_SETTINGS pset, P_SNAKE psnake);
P_SSEG generate_new_head(direction_t newdir, P_COORD newcoord);
void insert_new_head(P_SNAKE psnake, P_SSEG pnewhead);

bool process_char(int ch, WINDOW_SNAKE *ws, P_SETTINGS psettings, P_SNAKE psnake);

bool is_coord_on_snake(P_COORD pc_inq, P_SNAKE psnake);
void rank_coord(P_SSEG pseg, P_COORD *ppc_small, P_COORD *ppc_large);
bool is_coord_on_seg(P_COORD pc_inq, P_SSEG pseg);

void get_border_portal_coord(WINDOW_SNAKE *ws, P_SNAKE psnake,P_COORD pc);

void reverse_snake(P_SNAKE psnake);
direction_t get_oppose_dir(direction_t dir);

void show_status(WINDOW_SNAKE *ws, P_SETTINGS pset, P_SNAKE psnake);

inline NCURSES_SIZE_T y2graph(WINDOW_SNAKE *ws, NCURSES_SIZE_T ysnake);

/* Routines
 *************/
int main()
{
	WINDOW *w=NULL;
	WINDOW_SNAKE ws;
	
	P_SNAKE psnake = NULL;
	FOOD food;
	int ch = 0;
	SETTINGS settings;

	/* Initialize game's default settings */
	init_settings(&settings);

	/* Initialize ncurses */
	w = ncurses_init(&ws);
	
	/* Initialize the snake structure */
	psnake = snake_init(&ws);
	if( ! psnake ) {
		ncurses_uninit();
		return 1;
	}

	/* Draw the initial snake */
	snake_draw_init(&ws, &settings, psnake);
	getch();

	/* main loop */
	food.b_eaten = true;
	while ( !(ch == 'x')) {
		
		if(settings.b_altered) {
			show_status(&ws, &settings, psnake);
		}

		if(food.b_eaten) {
			place_food(&ws, &settings, &food ,psnake);
		}

		usleep((MAX_SPEED - settings.speed + 1) * DELAY_DELTA);

		if(!settings.pause) {
			if(!snake_move(&ws, &settings, psnake, &food)) {
				break;
			}
		}

		ch = tolower(getch());
		process_char(ch, &ws, &settings, psnake);
	}
	
	/* Uninitialize ncurses library */
	ncurses_uninit();

	/* Free all snake segments and snake structure */
	free_snake(psnake);
	return 0;
}

bool place_food(WINDOW_SNAKE *ws , P_SETTINGS pset, P_FOOD pfood, P_SNAKE psnake)
{
	long int rnd = 0;
	P_COORD pcoord = &pfood->coord;

	pfood->b_eaten = false;

	srandom(time(NULL));		

	/* Generate random location to place the food.
 	   If the random location turns out to be on snake's body
	   regenerate location
	*/
	do {
		pcoord->y = random() % (ws->_maxy);  
		pcoord->x = random() % (ws->_maxx);  
	} while(is_coord_on_snake(pcoord, psnake) || 
		pcoord->x < ws->_begx || 
	        pcoord->y < ws->_begy);

	/* Now draw the food */
	attron(COLOR_PAIR(COLOR_PAIR_FOOD));
	DRAW_CHAR(ws, pcoord->y, pcoord->x, pset->ch_food);
	attroff(COLOR_PAIR(COLOR_PAIR_FOOD));
}

bool process_char(int ch, WINDOW_SNAKE *ws, P_SETTINGS pset, P_SNAKE psnake)
{
	switch(ch) 
	{
		case '-':	
			if ( (pset->speed - 1) >= MIN_SPEED) {
				pset->speed--;
				pset->b_altered = true;
			}
			break;	
		case '+':
			if ( (pset->speed + 1) <= MAX_SPEED ) {
				pset->speed++;
				pset->b_altered = true;
			}
			break;	
		case 'p':
			pset->pause = pset->pause ? false : true;
			pset->b_altered = true;
			break;
		case 'o':
			pset->portal = pset->portal ? false: true;
			pset->b_altered = true;
			break;
		case 'c':
			pset->cheat = pset->cheat ? false: true;
			pset->b_altered = true;
			break;
		case 'v':
			pset->reverse = pset->reverse ? false: true;
			pset->b_altered = true;
			break;
		case 's':
			pset->sound = pset->sound ? false: true;
			pset->b_altered = true;
			break;
		case 'l':
		case KEY_LEFT:
			snake_steer(ws, pset, psnake, DIR_LEFT);
			pset->b_altered = true;
			break;
		case 'r':
		case KEY_RIGHT:
			snake_steer(ws, pset, psnake, DIR_RIGHT);
			pset->b_altered = true;
			break;
		case 'u':
		case KEY_UP:
			snake_steer(ws, pset, psnake, DIR_UP);
			pset->b_altered = true;
			break;
		case 'd':
		case KEY_DOWN:
			snake_steer(ws, pset, psnake, DIR_DOWN);
			pset->b_altered = true;
			break;
		case '\\':
			snake_steer(ws, pset, psnake, DIR_UP_LEFT);
			pset->b_altered = true;
			break;
		case 'q':
			snake_steer(ws, pset, psnake, DIR_DOWN_RIGHT);
			pset->b_altered = true;
			break;
		case '/':
			snake_steer(ws, pset, psnake, DIR_UP_RIGHT);
			pset->b_altered = true;
			break;
		case 'z':
			snake_steer(ws, pset, psnake, DIR_DOWN_LEFT);
			break;
		case 't':
			pset->ch_erase = (pset->ch_erase == DEFAULT_ERASE_CHAR) ? 
                                           DEFAULT_TRACE_CHAR :
				           DEFAULT_ERASE_CHAR;
			pset->b_altered = true;
			break;
		case 'g':
			pset->b_show_segcount = pset->b_show_segcount ? false : true; 
			if(pset->b_show_segcount) 
				pset->b_show_length = false;
			break;
		case '0':
			pset->b_show_length = pset->b_show_length ? false : true; 
			if(pset->b_show_length) 
				pset->b_show_segcount = false;
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
		case DIR_UP_LEFT:
			pcoord->x--;
			pcoord->y--;
			break;
		case DIR_UP_RIGHT:
			pcoord->x++;
			pcoord->y--;
			break;
		case DIR_DOWN_LEFT:
			pcoord->x--;
			pcoord->y++;
			break;
		case DIR_DOWN_RIGHT:
			pcoord->x++;
			pcoord->y++;
			break;
	}
}

bool seg_update_headxy(P_SSEG seg)
{
	seg_update_coord(seg->dir, &seg->coord_start);
}

bool seg_unupdate_headxy(P_SSEG seg)
{
	seg_update_coord(get_oppose_dir(seg->dir), &seg->coord_start);
}

bool seg_update_tailxy(P_SSEG seg)
{
	seg_update_coord(seg->dir, &seg->coord_end);
}

bool is_border(WINDOW_SNAKE *ws, P_SNAKE psnake)
{
	P_SSEG head = psnake->seg_head;
	P_COORD pcoord = &head->coord_start;

	if(pcoord->x >= ws->_begx && 
 	   pcoord->x <= ws->_maxx &&
	   pcoord->y >= ws->_begy &&
	   pcoord->y <= ws->_maxy) { 
		return false;
	}
	
	return true;
}

bool snake_steer(WINDOW_SNAKE *ws, P_SETTINGS pset, P_SNAKE psnake, direction_t new_dir)
{
	P_SSEG pnewhead = NULL;
	P_SSEG head = psnake->seg_head;
	direction_t curr_dir = head->dir; 

	if( curr_dir == new_dir ) {
		return true;
	}
	
	if(new_dir == get_oppose_dir(curr_dir)) {
		if( pset->reverse) {
			reverse_snake(psnake);	
		}
		return true;
	}
	
	pnewhead = generate_new_head(new_dir, &head->coord_start);
	if( ! pnewhead) {
		return false;
	}
	seg_update_tailxy(pnewhead);
	
	/* Make new segment the head of the snake */
	insert_new_head(psnake, pnewhead);

	return true;
}

P_SSEG generate_new_head(direction_t newdir, P_COORD newcoord)
{
	P_SSEG pnewhead = NULL;

	pnewhead = calloc(sizeof(SSEG), 1);
	if( ! pnewhead) {
		return false;
	}

	/* Setup the new segment in the expected direction */
	pnewhead->length = 0;	
	pnewhead->dir = newdir;
	pnewhead->coord_start.x = newcoord->x;
	pnewhead->coord_start.y = newcoord->y; 
	pnewhead->coord_end.x = newcoord->x;
	pnewhead->coord_end.y = newcoord->y;

	return pnewhead;

}

void insert_new_head(P_SNAKE psnake, P_SSEG pnewhead)
{
	/* First perform pointer surgeries on
           all involved nodes/segments and
           increment segment count
        */
	pnewhead->previous = NULL;
	if( psnake->seg_head->length) {
		pnewhead->next = psnake->seg_head;
		psnake->seg_head->previous = pnewhead;
		psnake->seg_count++;
	} 
	else {
		pnewhead->next = psnake->seg_head->next;
		if(pnewhead->next) {
			pnewhead->next->previous = pnewhead;
		}
		free(psnake->seg_head);
	}
	
	/* Now designate the new head
        */
	psnake->seg_head = pnewhead;
}

bool eat_food(P_SETTINGS pset, P_SNAKE psnake, P_FOOD pfood)
{
	P_COORD coord_head = &psnake->seg_head->coord_start;
	P_COORD coord_food = &pfood->coord;

	if(coord_head->y == coord_food->y &&
	   coord_head->x == coord_food->x) {
		pfood->b_eaten = true;
		psnake->score++;
		pset->b_altered = true; 
		if(pset->sound) {
			beep();
		}
		return true;
	}

	return false;
}

bool is_coord_on_snake(P_COORD pc_inq, P_SNAKE psnake)
{
	P_SSEG head = psnake->seg_head;
	P_SSEG eachseg = NULL;

	eachseg = head;
	while(eachseg) {
		if(is_coord_on_seg(pc_inq, eachseg))
			return true;
		eachseg = eachseg->next;
	}

	return false;	
}

bool is_self_collision(P_SETTINGS pset, P_SNAKE psnake)
{
	P_SSEG head = psnake->seg_head;
	P_COORD pc_inq = &head->coord_start;
	P_SSEG eachseg = NULL;

	if(pset->cheat) {
		return false;
	}

	if(psnake->seg_count == 1) {
		return false;
	}

	eachseg = head->next;
	while(eachseg) {
		if(is_coord_on_seg(pc_inq, eachseg))
			return true;
		eachseg = eachseg->next;
	}

	return false;	
}

void get_border_portal_coord(WINDOW_SNAKE *ws, P_SNAKE psnake,P_COORD pc)
{
	P_SSEG head = psnake->seg_head;
	direction_t dir = head->dir;
	P_COORD phead_coord = &head->coord_start;
	NCURSES_SIZE_T x, y, a, b;

	switch(dir) {
		case DIR_UP:
			pc->x = phead_coord->x;
			pc->y = ws->_maxy;
			break;	
		case DIR_DOWN:
			pc->x = phead_coord->x;
			pc->y = ws->_begy;
			break;	
		case DIR_LEFT:
			pc->y = phead_coord->y;
			pc->x = ws->_maxx;
			break;	
		case DIR_RIGHT:
			pc->y = phead_coord->y;
			pc->x = ws->_begx;
			break;	
		case DIR_UP_LEFT:
			//(y-b) = m(x-a), where,
			// m=-1, a=phead_coord->x, b=phead_coord->y
			// so, solve for x, when y=ws->_maxy;
			// Rearranging eq, we get
			// y - b = -x + a
			// x = b - y + a;
			a = phead_coord->x;
			b = y2graph(ws, phead_coord->y); 
			y = y2graph(ws, ws->_maxy);
			pc->x = b - y + a;
			pc->y = ws->_maxy;
			if(pc->x > ws->_maxx) {
				//if x > maxx then solve original again,
				//this time for y, when x = _maxx
				// which means, 
				// x = b - y + a, is now
				// y = b + a - x		
				x = ws->_maxx;
				pc->y = y2graph(ws, (b + a - x));
				pc->x = x;
			}
			break;
		case DIR_DOWN_RIGHT:
			//(y-b) = m(x-a), where,
			// m=-1, a=phead_coord->x, b=phead_coord->y
			// so, solve for x, when y=ws->_begy;
			// Rearranging eq, we get
			// y - b = -x + a
			// x = b - y + a;
			a = phead_coord->x;
			b = y2graph(ws, phead_coord->y); 
			y = y2graph(ws, ws->_begy);
			pc->x = b - y + a;
			pc->y = ws->_begy;
			if(pc->x < ws->_begx) {
				//if x < begx then solve original again,
				//this time for y, when x = _begx
				// which means, 
				// x = b - y + a, is now
				// y = b + a - x		
				x = ws->_begx;
				pc->y = y2graph(ws, (b + a - x));
				pc->x = x;
			}
			break;
		
		case DIR_UP_RIGHT:
			//(y-b) = m(x-a), where,
			// m=1, a=phead_coord->x, b=phead_coord->y
			// so, solve for x, where y=ws->_maxy;
			// Rearranging eq, we get
			// x = y - b + a;
			a = phead_coord->x;
			b = y2graph(ws, phead_coord->y); 
			y = y2graph(ws, ws->_maxy);
			pc->x = y - b + a;
			pc->y = ws->_maxy;
			if(pc-> x < ws->_begx) {
				//if x < begx then solve original again,
				//this time for y, when x = _begx
				// which means, 
				// x = y - b + a, is now
				// y = x + b - a		
				x = ws->_begx;
				pc->y = y2graph(ws, (x + b - a));
				pc->x = x;
			}
			break;
		case DIR_DOWN_LEFT:
			//(y-b) = m(x-a), where,
			// m=1, a=phead_coord->x, b=phead_coord->y
			// so, solve for x, where y=ws->_begy;
			// Rearranging eq, we get
			// x = y - b + a;
			a = phead_coord->x;
			b = y2graph(ws, phead_coord->y); 
			y = y2graph(ws, ws->_begy);
			pc->x = y - b + a;
			pc->y = ws->_begy;
			if(pc-> x > ws->_maxx) {
				//if x > maxx then solve original again,
				//this time for y, when x = _maxx
				// which means, 
				// x = y - b + a, is now
				// y = x + b - a		
				x = ws->_maxx;
				pc->y = y2graph(ws, (x + b - a));
				pc->x = x;
			break;
		}
	}
}

bool snake_move(WINDOW_SNAKE *ws, P_SETTINGS pset, P_SNAKE psnake, P_FOOD pfood)
{
	P_SSEG head = psnake->seg_head;
	P_SSEG tail = psnake->seg_tail;
        chtype ch = pset->b_show_segcount ?  (psnake->seg_count + '0') :
		    	pset->b_show_length ? head->length + '0'  : pset->ch_draw;	
	P_SSEG pnewhead = NULL;
	COORD newcoord = {0,0};

	/* Advance head's x,y (do not draw yet) */
	seg_update_headxy(head);

	/* Check if head is within the border */
	if( is_border(ws, psnake)) {

		/* If head hits border, and portal mode is OFF
 		   then quit the game
		*/
		if(!pset->portal) {
	        	sleep(2);	
			if(pset->sound) {
				beep();
			}
			return false;
		}

		/* Unadvance head's x,y */
		seg_unupdate_headxy(head);

		/* If head hits border, and portal mode is ON
		   then snake appear on the other side
		*/
		get_border_portal_coord(ws, psnake, &newcoord);
		pnewhead = generate_new_head(head->dir, &newcoord); 
		if( ! pnewhead) {
	        	sleep(2);	
			if(pset->sound) {
				beep();
			}
			return false;
		}
		insert_new_head(psnake, pnewhead);
		head = pnewhead;
	}

	/* Check if snake hs collided with itself */ 
	if( is_self_collision(pset, psnake)) {
	        sleep(2);	
		beep();
		return false;
	}

	/* Now draw the head of the snake at the new location */
	DRAW_SNAKE_HEAD(ws, 
               	head->coord_start.y, 
                head->coord_start.x, 
		ch);
	head->length++;

	/* Check if there was food at the new head position */
	if(eat_food(pset, psnake, pfood)) {
		/* If food was just eaten do not advance the tail
                   this will cause the snake to grow by one unit
		*/
		return true;
	}

	/* Actually draw advancement of the tail.
           Really this step clears (undraws) the very last character of the snake
        */
	DRAW_CHAR(ws, tail->coord_end.y, tail->coord_end.x, pset->ch_erase);
	tail->length--;


	/* If tail segment has finished, designate previous segment
           to be the new tail - and free the old tail
	*/
	if ( tail->length == 0 ) {
		psnake->seg_tail = tail->previous;	
		psnake->seg_tail->next = NULL;
		psnake->seg_count--;
		free(tail);
	} 
	else {
		/*  Advance tail's x,y (do not draw) */
		seg_update_tailxy(tail);
	}

	return true;
}

void init_settings(P_SETTINGS pset) 
{
	pset->speed = DEFAULT_SPEED;
	pset->pause = false;
	pset->portal = true;
	pset->cheat = false;
	pset->reverse = true;
	pset->sound = false;
	pset->ch_draw = DEFAULT_DRAW_CHAR;
	pset->ch_erase = DEFAULT_ERASE_CHAR;
	pset->ch_food = DEFAULT_FOOD_CHAR;
	pset->b_show_segcount = false;
        pset->b_show_length = false;
	
	pset->b_altered = true;
} 

P_SNAKE snake_init(WINDOW_SNAKE *ws)
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
	p_initseg->coord_start.x = ws->_maxx;
	p_initseg->coord_start.y = ws->_maxy - DEFAULT_INIT_LENGTH + 1;
	p_initseg->coord_end.x = p_initseg->coord_start.x;
	p_initseg->coord_end.y = ws->_maxy;

	/* Initialize snake */
	psnake->seg_count = 1;
	psnake->score = 0;

	/* Insert initial segment into the snake */
	psnake->seg_head = p_initseg;
	psnake->seg_tail = p_initseg;

	return psnake;
}

void snake_draw_init(WINDOW_SNAKE *ws, P_SETTINGS pset, P_SNAKE psnake)
{
	int i;
	P_SSEG pseg = psnake->seg_head; 
	NCURSES_SIZE_T y = pseg->coord_start.y;
	NCURSES_SIZE_T x = pseg->coord_start.x;

	for(i=0; i < pseg->length; i++, y++) {
		DRAW_SNAKE_HEAD(ws, y, x, pset->ch_draw);
	}
}

WINDOW* ncurses_init(P_WINDOW_SNAKE p_ws)
{
	WINDOW *w = NULL;
	
	w = initscr();
	keypad(stdscr, TRUE);
	cbreak();
	noecho();
	nonl();
	scrollok(w, false);
	nodelay(w, true);
	curs_set(0);

	start_color();
	init_pair(COLOR_PAIR_BOX, COLOR_CYAN, COLOR_BLACK);
	init_pair(COLOR_PAIR_FOOD, COLOR_GREEN, COLOR_BLACK);
	init_pair(COLOR_PAIR_SNAKE, COLOR_WHITE, COLOR_RED);
	init_pair(COLOR_PAIR_RED_ON_BLACK, COLOR_RED, COLOR_BLACK);
	init_pair(COLOR_PAIR_STATUS, COLOR_WHITE, COLOR_BLACK);

	/* Reserve space for key help */
	attron(COLOR_PAIR(COLOR_PAIR_BOX));
	box(w, '|','-');
	attroff(COLOR_PAIR(COLOR_PAIR_BOX));
	
	p_ws->_begy = w->_begy+1;
	p_ws->_begx = w->_begx+1;
	p_ws->_maxy = w->_maxy-2;   //Accomdate Status-bar
	p_ws->_maxx = w->_maxx-1;
	
	return w;
}

void rank_coord(P_SSEG pseg, P_COORD *ppc_small, P_COORD *ppc_large)
{
	if(pseg->coord_start.y > pseg->coord_end.y) {
		*ppc_large = &pseg->coord_start;	
		*ppc_small = &pseg->coord_end;	
		return;
	} 
	else if(pseg->coord_start.y < pseg->coord_end.y) {
		*ppc_large = &pseg->coord_end;	
		*ppc_small = &pseg->coord_start;	
		return;
	}
	else {
		if(pseg->coord_start.x > pseg->coord_end.x) {
			*ppc_large = &pseg->coord_start;	
			*ppc_small = &pseg->coord_end;	
			return;
		}
		else {
			//At this point start.x is either < or = end.x
			//If it is equal then, both - the start and the end cord -
			//are exactly the same	
			*ppc_large = &pseg->coord_end;	
			*ppc_small = &pseg->coord_start;	
			return;
		}
	}
}

bool is_coord_on_seg(P_COORD pc_inq, P_SSEG pseg)
{
	P_COORD pc_small=NULL, pc_large=NULL;
	if(pseg->dir == DIR_LEFT || pseg->dir == DIR_RIGHT) {
		if( pc_inq->y != pseg->coord_start.y) {
			return false;
		}
		rank_coord(pseg, &pc_small, &pc_large);
		if(pc_inq->x >= pc_small->x && pc_inq->x <= pc_large->x) {
			return true;
		} 
		else {
			return false;
		}
	} 	

	if(pseg->dir == DIR_UP || pseg->dir == DIR_DOWN) {
		if( pc_inq->x != pseg->coord_start.x) {
			return false;
		}
		rank_coord(pseg, &pc_small, &pc_large);
		if(pc_inq->y >= pc_small->y && pc_inq->y <= pc_large->y) {
			return true;
		} 
		else {
			return false;
		}
	} 	

	if(pseg->dir == DIR_UP_LEFT || pseg->dir == DIR_UP_RIGHT ||
	   pseg->dir == DIR_DOWN_LEFT || pseg->dir == DIR_DOWN_RIGHT) {
		return false;
	}

	//should never reach here
	printf("is_cord_on_page: hit no logic's land");
	assert(false);
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
		free(seg);
		seg = next;
	}

	free(psnake);
}

direction_t get_oppose_dir(direction_t dir)
{
	switch(dir) {
		case DIR_LEFT:
			return DIR_RIGHT;
		case DIR_RIGHT:
			return DIR_LEFT;
		case DIR_UP:
			return DIR_DOWN;
		case DIR_DOWN:
			return DIR_UP;
		case DIR_UP_LEFT:
			return DIR_DOWN_RIGHT;
		case DIR_UP_RIGHT:
			return DIR_DOWN_LEFT;
		case DIR_DOWN_LEFT:
			return DIR_UP_RIGHT;
		case DIR_DOWN_RIGHT:
			return DIR_UP_LEFT;
	}

	//Should never reach here !
	return DIR_UP;
}

void reverse_snake(P_SNAKE psnake)
{
	P_SSEG seg = psnake->seg_head;
	P_SSEG next = NULL;
	COORD coord_temp;
	
	while(seg) 
	{	
		next = seg->next;
		seg->next = seg->previous;
		seg->previous = next;
		seg->dir = get_oppose_dir(seg->dir);
		coord_temp = seg->coord_start;
		seg->coord_start = seg->coord_end;
		seg->coord_end = coord_temp;
		seg = next;
	}

	
	seg = psnake->seg_head;
	psnake->seg_head = psnake->seg_tail;
	psnake->seg_tail = seg; 
}

void show_status(WINDOW_SNAKE *ws, P_SETTINGS pset, P_SNAKE psnake)
{
	P_COORD pheadc = &psnake->seg_head->coord_start;
	P_COORD ptailc = &psnake->seg_tail->coord_end;
        char strbuff[100] ;
	char dir_char=0;

	move(ws->_maxy+1, ws->_begx);
	
	/*
	printw("head-len=%d, seg=%d, head @x,y = %d,%d, tail @x,y = %d,%d ", 
		psnake->seg_head->length,
		psnake->seg_count,
		pheadc->x, pheadc->y,
		ptailc->x, ptailc->y);
	*/

	attron(COLOR_PAIR(COLOR_PAIR_STATUS));
	switch(psnake->seg_head->dir) {
		case DIR_UP:
			dir_char = '^';
			break;
		case DIR_DOWN:
			dir_char = 'v';
			break;
		case DIR_LEFT:
			dir_char = '<';
			break;
		case DIR_RIGHT:
			dir_char = '>';
			break;
		case DIR_UP_LEFT:
		case DIR_DOWN_RIGHT:
			dir_char = '\\';
			break;
		case DIR_UP_RIGHT:
		case DIR_DOWN_LEFT:
			dir_char = '/';
			break;
	}
	snprintf(strbuff,sizeof(strbuff),"%c",dir_char);
	attron(COLOR_PAIR(COLOR_PAIR_RED_ON_BLACK) | STATUS_DIR);
	addstr(strbuff);
	attroff(COLOR_PAIR(COLOR_PAIR_RED_ON_BLACK) | STATUS_DIR);
	//addstr("   ");	
	
	//addstr("speed");
	snprintf(strbuff, sizeof(strbuff), "%d", pset->speed);
        addstr(strbuff);
	addstr("(");
	if(pset->speed > MIN_SPEED)  
		attron(STATUS_SPEED_AVAIL);
	else
		attroff(STATUS_SPEED_AVAIL);
	addstr("-");
	if(pset->speed < MAX_SPEED)
		attron(STATUS_SPEED_AVAIL);
	else
		attroff(STATUS_SPEED_AVAIL);
	addstr("+");
	attroff(STATUS_SPEED_AVAIL);
	addstr(")");
	addstr("   ");	

	snprintf(strbuff, sizeof(strbuff), "%05d",psnake->score);
	attron(COLOR_PAIR(COLOR_PAIR_FOOD));
	addstr("@");
	attroff(COLOR_PAIR(COLOR_PAIR_FOOD));
	addstr(strbuff);
	addstr("   ");	
	

	if(pset->sound) attron(STATUS_ATTR);
	addstr("(s)ound");	
	if(pset->sound) attroff(STATUS_ATTR);
	addstr("   ");	

	if(pset->pause) {
		attron(STATUS_ATTR);
		addstr("un(p)ause");	
		attroff(STATUS_ATTR);
	} 
	else
	{
		addstr("(p)ause");	
	}
	addstr("   ");	

	if(pset->portal) attron(STATUS_ATTR);
	addstr("p(o)rtal");	
	if(pset->portal) attroff(STATUS_ATTR);
	addstr("   ");	

	if(pset->reverse) attron(STATUS_ATTR);
	addstr("re(v)erse");	
	if(pset->reverse) attroff(STATUS_ATTR);
	addstr("   ");	

	if(pset->cheat) attron(STATUS_ATTR);
	addstr("(c)heat");	
	if(pset->cheat) attroff(STATUS_ATTR);
	addstr("   ");	

	addstr("e(x)it");	
	addstr("   ");	

	attroff(COLOR_PAIR(COLOR_PAIR_STATUS));
	
	pset->b_altered = false;
}

inline NCURSES_SIZE_T y2graph(WINDOW_SNAKE *ws, NCURSES_SIZE_T ysnake)
{
	return (ws->_maxy - (ysnake));	
}

